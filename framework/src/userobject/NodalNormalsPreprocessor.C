/****************************************************************/
/*               DO NOT MODIFY THIS HEADER                      */
/* MOOSE - Multiphysics Object Oriented Simulation Environment  */
/*                                                              */
/*           (c) 2010 Battelle Energy Alliance, LLC             */
/*                   ALL RIGHTS RESERVED                        */
/*                                                              */
/*          Prepared by Battelle Energy Alliance, LLC           */
/*            Under Contract No. DE-AC07-05ID14517              */
/*            With the U. S. Department of Energy               */
/*                                                              */
/*            See COPYRIGHT for full restrictions               */
/****************************************************************/

// MOOSE includes
#include "NodalNormalsPreprocessor.h"
#include "Assembly.h"
#include "MooseMesh.h"

// libmesh includes
#include "libmesh/quadrature.h"

Threads::spin_mutex nodal_normals_preprocessor_mutex;

template<>
InputParameters validParams<NodalNormalsPreprocessor>()
{
  InputParameters params = validParams<ElementUserObject>();
  params += validParams<BoundaryRestrictable>();
  params.addParam<BoundaryName>("corner_boundary", "Node set ID which contains the nodes that are in 'corners'.");
  params.addPrivateParam<FEFamily>("fe_family", LAGRANGE);
  params.addPrivateParam<Order>("fe_order", FIRST);

  return params;
}

NodalNormalsPreprocessor::NodalNormalsPreprocessor(const InputParameters & parameters) :
    ElementUserObject(parameters),
    BoundaryRestrictable(parameters, true), // true for applying to nodesets
    _aux(_fe_problem.getAuxiliarySystem()),
    _fe_type(getParam<Order>("fe_order"), getParam<FEFamily>("fe_family")),
    _has_corners(isParamValid("corner_boundary")),
    _corner_boundary_id(_has_corners ? _mesh.getBoundaryID(getParam<BoundaryName>("corner_boundary")) : static_cast<BoundaryID>(-1)),
    _grad_phi(_assembly.feGradPhi(_fe_type))
{
}

NodalNormalsPreprocessor::~NodalNormalsPreprocessor()
{
}

void
NodalNormalsPreprocessor::initialize()
{
  NumericVector<Number> & sln = _aux.solution();
  _aux.system().zero_variable(sln, _aux.getVariable(_tid, "nodal_normal_x").number());
  _aux.system().zero_variable(sln, _aux.getVariable(_tid, "nodal_normal_y").number());
  _aux.system().zero_variable(sln, _aux.getVariable(_tid, "nodal_normal_z").number());
}

void
NodalNormalsPreprocessor::execute()
{
  NumericVector<Number> & sln = _aux.solution();

  // Get a reference to our BoundaryInfo object for later use...
  BoundaryInfo & boundary_info = _mesh.getMesh().get_boundary_info();

  // Container to catch IDs handed back by BoundaryInfo.
  std::vector<BoundaryID> node_boundary_ids;

  // Loop through each node on the current element
  for (unsigned int i = 0; i < _current_elem->n_nodes(); i++)
  {
    // Extract a pointer to a node
    const Node * node = _current_elem->node_ptr(i);

    // Only continue if the node is on a boundary
    if (_mesh.isBoundaryNode(node->id()))
    {
      // List of IDs for the boundary
      boundary_info.boundary_ids(node, node_boundary_ids);

      // Perform the calculation, the node must be:
      //    (1) On a boundary to which the object is restricted
      //    (2) Not on a corner of the boundary
      if (hasBoundary(node_boundary_ids, ANY)
          && (!_has_corners || !boundary_info.has_boundary_id(node, _corner_boundary_id)))
      {
        // Perform the caluation of the normal
        if (node->n_dofs(_aux.number(), _fe_problem.getVariable(_tid, "nodal_normal_x").number()) > 0)
        {
          // but it is not a corner node, they will be treated differently later on
          dof_id_type dof_x = node->dof_number(_aux.number(), _fe_problem.getVariable(_tid, "nodal_normal_x").number(), 0);
          dof_id_type dof_y = node->dof_number(_aux.number(), _fe_problem.getVariable(_tid, "nodal_normal_y").number(), 0);
          dof_id_type dof_z = node->dof_number(_aux.number(), _fe_problem.getVariable(_tid, "nodal_normal_z").number(), 0);

          for (unsigned int qp = 0; qp < _qrule->n_points(); qp++)
          {
            Threads::spin_mutex::scoped_lock lock(nodal_normals_preprocessor_mutex);

            sln.add(dof_x, _JxW[qp] * _grad_phi[i][qp](0));
            sln.add(dof_y, _JxW[qp] * _grad_phi[i][qp](1));
            sln.add(dof_z, _JxW[qp] * _grad_phi[i][qp](2));
          }
        }
      }
    }
  }
}

void
NodalNormalsPreprocessor::finalize()
{
  _aux.solution().close();
}

void
NodalNormalsPreprocessor::threadJoin(const UserObject & /*uo*/)
{
}
