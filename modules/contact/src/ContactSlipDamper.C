/****************************************************************/
/* MOOSE - Multiphysics Object Oriented Simulation Environment  */
/*                                                              */
/*          All contents are licensed under LGPL V2.1           */
/*             See LICENSE for full restrictions                */
/****************************************************************/

#include "ContactSlipDamper.h"
#include "FEProblem.h"
#include "DisplacedProblem.h"
#include "AuxiliarySystem.h"
#include "PenetrationLocator.h"
#include "NearestNodeLocator.h"

template<>
InputParameters validParams<ContactSlipDamper>()
{
  InputParameters params = validParams<GeneralDamper>();
  params.addRequiredParam<std::vector<int> >("master", "IDs of the master surfaces for which slip reversals should be damped");
  params.addRequiredParam<std::vector<int> >("slave", "IDs of the slave surfaces for which slip reversals should be damped");
  params.addParam<Real>("max_iterative_slip", std::numeric_limits<Real>::max(), "Maximum iterative slip");
  params.addRangeCheckedParam<Real>("min_damping_factor", 0.0, "min_damping_factor < 1.0", "Minimum permissible value for damping factor");
  params.addParam<Real>("damping_threshold_factor", 1.0e3, "If previous iterations's slip is below the slip tolerance, only damp a slip reversal if the slip magnitude is greater than than this factor times the old slip.");
  params.addParam<bool>("debug_output", false, "Output detailed debugging information");
  return params;
}

ContactSlipDamper::ContactSlipDamper(const InputParameters & parameters) :
    GeneralDamper(parameters),
    _aux_sys(parameters.get<FEProblem *>("_fe_problem")->getAuxiliarySystem()),
    _displaced_problem(parameters.get<FEProblem *>("_fe_problem")->getDisplacedProblem()),
    _num_contact_nodes(0),
    _num_sticking(0),
    _num_slipping(0),
    _num_slipping_friction(0),
    _num_stick_locked(0),
    _num_slip_reversed(0),
    _max_iterative_slip(parameters.get<Real>("max_iterative_slip")),
    _min_damping_factor(parameters.get<Real>("min_damping_factor")),
    _damping_threshold_factor(parameters.get<Real>("damping_threshold_factor")),
    _debug_output(parameters.get<bool>("debug_output"))
{
  if (!_displaced_problem)
    mooseError("Must have displaced problem to use ContactSlipDamper");

  std::vector<int> master = parameters.get<std::vector<int> >("master");
  std::vector<int> slave = parameters.get<std::vector<int> >("slave");

  unsigned int num_interactions = master.size();
  if (num_interactions != slave.size())
    mooseError("Sizes of master surface and slave surface lists must match in ContactSlipDamper");
  if (num_interactions == 0)
    mooseError("Must define at least one master/slave pair in ContactSlipDamper");

  for (unsigned int i = 0; i < master.size(); ++i)
  {
    std::pair<int, int> ms_pair(master[i], slave[i]);
    _interactions.insert(ms_pair);
  }
}

void
ContactSlipDamper::timestepSetup()
{
  GeometricSearchData & displaced_geom_search_data = _displaced_problem->geomSearchData();
  std::map<std::pair<unsigned int, unsigned int>, PenetrationLocator *> * penetration_locators = &displaced_geom_search_data._penetration_locators;

  for (pl_iterator plit = penetration_locators->begin(); plit != penetration_locators->end(); ++plit)
  {
    PenetrationLocator & pen_loc = *plit->second;

    if (operateOnThisInteraction(pen_loc))
    {
      std::vector<dof_id_type> & slave_nodes = pen_loc._nearest_node._slave_nodes;

      for (unsigned int i = 0; i < slave_nodes.size(); i++)
      {
        dof_id_type slave_node_num = slave_nodes[i];

        if (pen_loc._penetration_info[slave_node_num])
        {
          PenetrationInfo & info = *pen_loc._penetration_info[slave_node_num];
          const Node * node = info._node;

          if (node->processor_id() == processor_id())
//              && info.isCaptured())                  //TODO maybe just set this everywhere?
            info._slip_reversed = false;
        }
      }
    }
  }
}

Real
ContactSlipDamper::computeDamping(const NumericVector<Number> & solution,
                                  const NumericVector<Number> & /*update*/)
{
  //Do new contact search to update positions of slipped nodes
  _displaced_problem->updateMesh(solution, *_aux_sys.currentSolution());

  Real damping = 1.0;

  _num_contact_nodes = 0;
  _num_sticking = 0;
  _num_slipping = 0;
  _num_slipping_friction = 0;
  _num_stick_locked = 0;
  _num_slip_reversed = 0;

  GeometricSearchData & displaced_geom_search_data = _displaced_problem->geomSearchData();
  std::map<std::pair<unsigned int, unsigned int>, PenetrationLocator *> * penetration_locators = &displaced_geom_search_data._penetration_locators;

  for (pl_iterator plit = penetration_locators->begin(); plit != penetration_locators->end(); ++plit)
  {
    PenetrationLocator & pen_loc = *plit->second;

    if (operateOnThisInteraction(pen_loc))
    {
      std::vector<dof_id_type> & slave_nodes = pen_loc._nearest_node._slave_nodes;

      for (unsigned int i = 0; i < slave_nodes.size(); i++)
      {
        dof_id_type slave_node_num = slave_nodes[i];

        if (pen_loc._penetration_info[slave_node_num])
        {
          PenetrationInfo & info = *pen_loc._penetration_info[slave_node_num];
          const Node * node = info._node;

          if (node->processor_id() == processor_id())
          {
            if (info.isCaptured())
            {
              _num_contact_nodes++;
              if (info._mech_status == PenetrationInfo::MS_STICKING)
                _num_sticking++;
              else if (info._mech_status == PenetrationInfo::MS_SLIPPING)
                _num_slipping++;
              else if (info._mech_status == PenetrationInfo::MS_SLIPPING_FRICTION)
                _num_slipping_friction++;
              if (info._stick_locked_this_step >= 2) //TODO get from contact interaction
                _num_stick_locked++;

              RealVectorValue tangential_inc_slip_prev_iter = info._incremental_slip_prev_iter -
                                                              (info._incremental_slip_prev_iter * info._normal) * info._normal;
              RealVectorValue tangential_inc_slip = info._incremental_slip -
                                                    (info._incremental_slip * info._normal) * info._normal;

              RealVectorValue tangential_it_slip = tangential_inc_slip - tangential_inc_slip_prev_iter;
              Real node_damping_factor = 1.0;
              if ((tangential_inc_slip_prev_iter * tangential_inc_slip < 0.0) &&
                  info._mech_status == PenetrationInfo::MS_SLIPPING_FRICTION)
              {
                info._slip_reversed = true;
                _num_slip_reversed++;
                Real prev_iter_slip_mag = tangential_inc_slip_prev_iter.norm();
                RealVectorValue prev_iter_slip_dir = tangential_inc_slip_prev_iter / prev_iter_slip_mag;
                Real cur_it_slip_in_old_dir = tangential_it_slip * prev_iter_slip_dir;

                if (prev_iter_slip_mag > info._slip_tol ||
                    cur_it_slip_in_old_dir > -_damping_threshold_factor * prev_iter_slip_mag)
                  node_damping_factor = 1.0 - (cur_it_slip_in_old_dir + prev_iter_slip_mag) / cur_it_slip_in_old_dir;

                if (node_damping_factor < 0.0)
                  mooseError("Damping factor can't be negative");

                if (node_damping_factor < _min_damping_factor)
                  node_damping_factor = _min_damping_factor;
              }

              if (tangential_it_slip.norm() > _max_iterative_slip)
                node_damping_factor = (tangential_it_slip.norm() - _max_iterative_slip) / tangential_it_slip.norm();

              if (_debug_output && node_damping_factor < 1.0)
                _console << "Damping node: " << node->id()
                  << " prev iter slip: " << info._incremental_slip_prev_iter
                  << " curr iter slip: "<< info._incremental_slip
                  << " slip_tol: " << info._slip_tol
                  << " damping factor: " << node_damping_factor
                  << "\n";

              if (node_damping_factor < damping)
                damping = node_damping_factor;
            }
          }
        }
      }
    }
    _console << std::flush;
    _communicator.sum(_num_contact_nodes);
    _communicator.sum(_num_sticking);
    _communicator.sum(_num_slipping);
    _communicator.sum(_num_slipping_friction);
    _communicator.sum(_num_stick_locked);
    _communicator.sum(_num_slip_reversed);
    _communicator.min(damping);
  }

  _console << "   ContactSlipDamper: Damping     #Cont    #Stick     #Slip #SlipFric #StickLock  #SlipRev\n";

  _console << std::right
           << std::setw(29) << damping
           << std::setw(10) << _num_contact_nodes
           << std::setw(10) << _num_sticking
           << std::setw(10) << _num_slipping
           << std::setw(10) << _num_slipping_friction
           << std::setw(11) << _num_stick_locked
           << std::setw(10) << _num_slip_reversed
           << "\n\n";
  _console << std::flush;

  return damping;
}

bool
ContactSlipDamper::operateOnThisInteraction(const PenetrationLocator & pen_loc)
{
  bool operate_on_this_interaction = false;
  std::set<std::pair<int, int> >::iterator ipit;
  std::pair<int, int> ms_pair(pen_loc._master_boundary, pen_loc._slave_boundary);
  ipit = _interactions.find(ms_pair);
  if (ipit != _interactions.end())
    operate_on_this_interaction = true;
  return operate_on_this_interaction;
}
