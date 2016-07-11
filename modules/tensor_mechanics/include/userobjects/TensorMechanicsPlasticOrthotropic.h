/****************************************************************/
/* MOOSE - Multiphysics Object Oriented Simulation Environment  */
/*                                                              */
/*          All contents are licensed under LGPL V2.1           */
/*             See LICENSE for full restrictions                */
/****************************************************************/
#ifndef TENSORMECHANICSPLASTICORTHOTROPIC_H
#define TENSORMECHANICSPLASTICORTHOTROPIC_H

#include "TensorMechanicsPlasticIsotropicSD.h"

class TensorMechanicsPlasticOrthotropic;

template<>
InputParameters validParams<TensorMechanicsPlasticOrthotropic>();

/**
* Orthotropic plasticity model from Yoon (2013)
* the name of the paper is "Asymmetric yield function based on the
* stress invariants for pressure sensitive metals" published
* 4th December 2013.
* This model accounts for sensitivity in pressure, the
* strength differential effect and orthotropic behavior
* Yield_function = b*I1 + (J'2^{3/2} - c*J''3)^{1/3} - yield_strength
* The last three functions are the main functions that call all other
* functions in this module for the Newton-Raphson method.
*/

class TensorMechanicsPlasticOrthotropic : public TensorMechanicsPlasticIsotropicSD
{
public:
  TensorMechanicsPlasticOrthotropic(const InputParameters & parameters);

protected:

  const std::vector<Real> _c1;

  const std::vector<Real> _c2;

  RankFourTensor _l1;

  RankFourTensor _l2;

  //Yield_function = a[b*I1 + (J2^{3/2} - c*J3)^{1/3}] - yield_strength
  Real yieldFunction(const RankTwoTensor & stress, Real intnl) const override;

  //Tensor derivative of the yield_function with respect to the stress tensor
  RankTwoTensor dyieldFunction_dstress(const RankTwoTensor & stress, Real intnl) const override;

  //Tensor derivative of the tensor derivative of the yield_function with respect to the stress tensor
  RankFourTensor dflowPotential_dstress(const RankTwoTensor & stress, Real intnl) const override;

  RankTwoTensor flowPotential(const RankTwoTensor & stress, Real intnl) const override;

};

#endif // TENSORMECHANICSPLASTICORTOTHROPIC_H
