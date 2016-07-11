# UserObject IsotropicSD test
# IsotropicSD plasticity model from Yoon (2013)
# the name of the paper is "Asymmetric yield function based on the
# stress invariants for pressure sensitive metals" published
# 4th December 2013.
# This model accounts for sensitivity in pressure and for the
# strength differential effect
# Yield_function = a[b*I1 + (J2^{3/2} - c*J3)^{1/3}] - yield_strength

[Mesh]
  type = GeneratedMesh
  dim = 3
  nx = 1
  ny = 1
  nz = 1
  xmin =  -.5
  xmax = .5
  ymin = -.5
  ymax = .5
  zmin = -.5
  zmax = .5
[]


[Variables]
  [./disp_x]
  [../]
  [./disp_y]
  [../]
  [./disp_z]
  [../]
[]

[Kernels]
  [./TensorMechanics]
    displacements = 'disp_x disp_y disp_z'
  [../]
[]


[BCs]
  [./xdisp]
    type = FunctionPresetBC
    variable = disp_x
    boundary = 'right'
    function = '0.01*t'
  [../]
  [./yfix]
    type = PresetBC
    variable = disp_y
    boundary = 'bottom'
    value = 0
  [../]
  [./xfix]
    type = PresetBC
    variable = disp_x
    boundary = 'left'
    value = 0
  [../]
  [./zfix]
    type = PresetBC
    variable = disp_z
    boundary = 'back'
    value = 0
  [../]
[]

[AuxVariables]
  [./stress_xx]
    order = CONSTANT
    family = MONOMIAL
  [../]
  [./stress_xy]
    order = CONSTANT
    family = MONOMIAL
  [../]
  [./stress_xz]
    order = CONSTANT
    family = MONOMIAL
  [../]
  [./stress_yy]
    order = CONSTANT
    family = MONOMIAL
  [../]
  [./stress_yz]
    order = CONSTANT
    family = MONOMIAL
  [../]
  [./stress_zz]
    order = CONSTANT
    family = MONOMIAL
  [../]
  [./f]
    order = CONSTANT
    family = MONOMIAL
  [../]
  [./iter]
    order = CONSTANT
    family = MONOMIAL
  [../]
  [./intnl]
    order = CONSTANT
    family = MONOMIAL
  [../]
  [./sdev]
    order = CONSTANT
    family = MONOMIAL
  [../]
  [./sh]
    order = CONSTANT
    family = MONOMIAL
  [../]
  [./sdet]
    order = CONSTANT
    family = MONOMIAL
  [../]
[]

[AuxKernels]
  [./stress_xx]
    type = RankTwoAux
    rank_two_tensor = stress
    variable = stress_xx
    index_i = 0
    index_j = 0
  [../]
  [./stress_xy]
    type = RankTwoAux
    rank_two_tensor = stress
    variable = stress_xy
    index_i = 0
    index_j = 1
  [../]
  [./stress_xz]
    type = RankTwoAux
    rank_two_tensor = stress
    variable = stress_xz
    index_i = 0
    index_j = 2
  [../]
  [./stress_yy]
    type = RankTwoAux
    rank_two_tensor = stress
    variable = stress_yy
    index_i = 1
    index_j = 1
  [../]
  [./stress_yz]
    type = RankTwoAux
    rank_two_tensor = stress
    variable = stress_yz
    index_i = 1
    index_j = 2
  [../]
  [./stress_zz]
    type = RankTwoAux
    rank_two_tensor = stress
    variable = stress_zz
    index_i = 2
    index_j = 2
  [../]
  [./f]
    type = MaterialStdVectorAux
    index = 0
    property = plastic_yield_function
    variable = f
  [../]
  [./iter]
    type = MaterialRealAux
    property = plastic_NR_iterations
    variable = iter
  [../]
  [./intnl]
    type = MaterialStdVectorAux
    index = 0
    property = plastic_internal_parameter
    variable = intnl
  [../]
  [./sdev]
    type = RankTwoScalarAux
    variable = sdev
    rank_two_tensor = stress
    scalar_type = VonMisesStress
  [../]
[]

[Postprocessors]
  [./s_xx]
    type = PointValue
    point = '0 0 0'
    variable = stress_xx
  [../]
  [./s_xy]
    type = PointValue
    point = '0 0 0'
    variable = stress_xy
  [../]
  [./s_xz]
    type = PointValue
    point = '0 0 0'
    variable = stress_xz
  [../]
  [./s_yy]
    type = PointValue
    point = '0 0 0'
    variable = stress_yy
  [../]
  [./s_yz]
    type = PointValue
    point = '0 0 0'
    variable = stress_yz
  [../]
  [./s_zz]
    type = PointValue
    point = '0 0 0'
    variable = stress_zz
  [../]
  [./intnl]
    type = PointValue
    point = '0 0 0'
    variable = intnl
  [../]
[]

[UserObjects]
  [./str]
    type = TensorMechanicsHardeningConstant
    value = 300
  [../]
  [./IsotropicSD]
    type = TensorMechanicsPlasticIsotropicSD
    b = 0.1
    c = 1.29903811
    associative = true
    yield_strength = str
    yield_function_tolerance = 1e-5
    internal_constraint_tolerance = 1e-9
    use_custom_returnMap = false
    use_custom_cto = false
  [../]
[]

[Materials]
  [./elasticity_tensor]
    type = ComputeElasticityTensor
    block = 0
    fill_method = symmetric_isotropic
    C_ijkl = '121e3 80e3'
  [../]
  [./strain]
    type = ComputeFiniteStrain
    block = 0
    displacements = 'disp_x disp_y disp_z'
  [../]
  [./mc]
    type = ComputeMultiPlasticityStress
    block = 0
    ep_plastic_tolerance = 1e-9
    plastic_models = IsotropicSD
    debug_fspb = crash
    tangent_operator = elastic
  [../]
[]


[Executioner]
  num_steps = 5
  dt = .05
  type = Transient

  nl_rel_tol = 1e-6
  nl_max_its = 10
  l_tol = 1e-4
  l_max_its = 50

  solve_type = NEWTON
  petsc_options_iname = '-pc_type -pc_factor_mat_solver_package -ksp_gmres_restart -snes_ksp_ew_rtol0 -snes_ksp_ew_rtolmax -snes_ksp_ew_gamma -snes_ksp_ew_alpha -snes_ksp_ew_alpha2 -snes_ksp_ew_threshold'
  petsc_options_value = 'lu superlu_dist 51 0.5 0.9 1 2 2 0.1'
[]


[Outputs]
  print_perf_log = true
  exodus = true
  csv = true
[]

[Preconditioning]
 [./smp]
  type = SMP
  full = true
 [../]
[]
