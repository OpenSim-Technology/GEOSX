/*
 * ------------------------------------------------------------------------------------------------------------
 * SPDX-License-Identifier: LGPL-2.1-only
 *
 * Copyright (c) 2018-2020 Lawrence Livermore National Security LLC
 * Copyright (c) 2018-2020 The Board of Trustees of the Leland Stanford Junior University
 * Copyright (c) 2018-2020 Total, S.A
 * Copyright (c) 2019-     GEOSX Contributors
 * All rights reserved
 *
 * See top level LICENSE, COPYRIGHT, CONTRIBUTORS, NOTICE, and ACKNOWLEDGEMENTS files for details.
 * ------------------------------------------------------------------------------------------------------------
 */

/**
 * @file PhaseFieldDamageKernels.hpp
 */

#ifndef GEOSX_PHYSICSSOLVERS_SIMPLEPDE_PHASEFIELDDAMAGEKERNELS_HPP_
#define GEOSX_PHYSICSSOLVERS_SIMPLEPDE_PHASEFIELDDAMAGEKERNELS_HPP_

#include "finiteElement/kernelInterface/ImplicitKernelBase.hpp"

namespace geosx
{
//*****************************************************************************
/**
 * @brief Implements kernels for solving the Damage(or phase-field) equation
 * in a phase-field fracture problem.
 * @copydoc geosx::finiteElement::KernelBase
 * @tparam NUM_NODES_PER_ELEM The number of nodes per element for the
 *                            @p SUBREGION_TYPE.
 * @tparam UNUSED An unused parameter since we are assuming that the test and
 *                trial space have the same number of support points.
 *
 * ### PhaseFieldDamageKernel Description
 * Implements the KernelBase interface functions required for solving the
 * Damage(or phase-field) equation in a phase-field fracture problem.
 * It uses the finite element kernel application functions such as
 * geosx::finiteElement::RegionBasedKernelApplication.
 *
 * In this implementation, the template parameter @p NUM_NODES_PER_ELEM is used
 * in place of both @p NUM_TEST_SUPPORT_POINTS_PER_ELEM and
 * @p NUM_TRIAL_SUPPORT_POINTS_PER_ELEM, which are assumed to be equal. This
 * results in the @p UNUSED template parameter as only the NUM_NODES_PER_ELEM
 * is passed to the ImplicitKernelBase template to form the base class.
 *
 * Additionally, the number of degrees of freedom per support point for both
 * the test and trial spaces are specified as `1` when specifying the base
 * class.
 */
template< typename SUBREGION_TYPE,
          typename CONSTITUTIVE_TYPE,
          typename FE_TYPE >
class PhaseFieldDamageKernel :
  public finiteElement::ImplicitKernelBase< SUBREGION_TYPE,
                                            CONSTITUTIVE_TYPE,
                                            FE_TYPE,
                                            1,
                                            1 >
{
public:
  /// An alias for the base class.
  using Base = finiteElement::ImplicitKernelBase< SUBREGION_TYPE,
                                                  CONSTITUTIVE_TYPE,
                                                  FE_TYPE,
                                                  1,
                                                  1 >;

  using Base::numDofPerTestSupportPoint;
  using Base::numDofPerTrialSupportPoint;
  using Base::m_dofNumber;
  using Base::m_dofRankOffset;
  using Base::m_matrix;
  using Base::m_rhs;
  using Base::m_elemsToNodes;
  using Base::m_constitutiveUpdate;

  /// The number of nodes per element.
  static constexpr int numNodesPerElem = Base::numTestSupportPointsPerElem;

  /**
   * @brief Constructor
   * @copydoc geosx::finiteElement::ImplicitKernelBase::ImplicitKernelBase
   * @param fieldName The name of the primary field
   *                  (i.e. Temperature, Pressure, etc.)
   */
  PhaseFieldDamageKernel( NodeManager const & nodeManager,
                          EdgeManager const & edgeManager,
                          FaceManager const & faceManager,
                          SUBREGION_TYPE const & elementSubRegion,
                          FE_TYPE const & finiteElementSpace,
                          CONSTITUTIVE_TYPE * const inputConstitutiveType,
                          arrayView1d< globalIndex const > const & inputDofNumber,
                          globalIndex const rankOffset,
                          CRSMatrixView< real64, globalIndex const > const & inputMatrix,
                          arrayView1d< real64 > const & inputRhs,
                          string const & fieldName,
                          real64 const Gc,
                          real64 const lengthScale,
                          int const localDissipationOption ):
    Base( nodeManager,
          edgeManager,
          faceManager,
          elementSubRegion,
          finiteElementSpace,
          inputConstitutiveType,
          inputDofNumber,
          rankOffset,
          inputMatrix,
          inputRhs ),
    m_nodalDamage( nodeManager.template getReference< array1d< real64 > >( fieldName )),
    m_dNdX( elementSubRegion.dNdX() ),
    m_detJ( elementSubRegion.detJ() ),
    m_Gc( Gc ),
    m_lengthScale( lengthScale ),
    m_localDissipationOption( localDissipationOption )
  {}

  //***************************************************************************
  /**
   * @class StackVariables
   * @copydoc geosx::finiteElement::ImplicitKernelBase::StackVariables
   *
   * Adds a stack array for the primary field.
   */
  struct StackVariables : Base::StackVariables
  {
public:

    /**
     * @brief Constructor
     */
    GEOSX_HOST_DEVICE
    StackVariables():
      Base::StackVariables(),
            nodalDamageLocal{ 0.0 }
    {}

    /// C-array storage for the element local primary field variable.
    real64 nodalDamageLocal[numNodesPerElem];
  };


  /**
   * @brief Copy global values from primary field to a local stack array.
   * @copydoc geosx::finiteElement::ImplicitKernelBase::setup
   *
   * For the PhaseFieldDamageKernel implementation, global values from the
   * primaryField, and degree of freedom numbers are placed into element local
   * stack storage.
   */
  GEOSX_HOST_DEVICE
  GEOSX_FORCE_INLINE
  void setup( localIndex const k,
              StackVariables & stack ) const
  {
    for( localIndex a=0; a<numNodesPerElem; ++a )
    {
      localIndex const localNodeIndex = m_elemsToNodes( k, a );

      stack.nodalDamageLocal[ a ] = m_nodalDamage[ localNodeIndex ];
      stack.localRowDofIndex[a] = m_dofNumber[localNodeIndex];
      stack.localColDofIndex[a] = m_dofNumber[localNodeIndex];
    }
  }

  /**
   * @copydoc geosx::finiteElement::ImplicitKernelBase::quadraturePointJacobianContribution
   */
  GEOSX_HOST_DEVICE
  GEOSX_FORCE_INLINE
  void quadraturePointKernel( localIndex const k,
                                            localIndex const q,
                                            StackVariables & stack ) const
  {

    real64 const strainEnergyDensity = m_constitutiveUpdate.calculateStrainEnergyDensity( k, q );
//    std::cout<<k<<", "<<q<<", "<< strainEnergyDensity<<std::endl;
    real64 D = 0;                                                                   //max between threshold and
                                                                                    // Elastic energy
    if( m_localDissipationOption == 1 )
    {
      double const threshold = 3 * m_Gc / (16 * m_lengthScale);           //elastic energy threshold - use when Local Dissipation is linear
      D = fmax( threshold, strainEnergyDensity );
      //D = max(strainEnergy(k,q), strainEnergy(k,q));//debbuging line - remove after testing
    }

    //Interpolate d and grad_d
    real64 N[ numNodesPerElem ];
    FE_TYPE::calcN( q, N );

    real64 qp_damage = 0.0;
    R1Tensor qp_grad_damage;
    R1Tensor temp;
    // TODO replace with FEM operators once either PR is merged into develop.
    for( localIndex a = 0; a < numNodesPerElem; ++a )
    {
      qp_damage += N[a] * stack.nodalDamageLocal[a];
      temp = m_dNdX[k][q][a];
      temp *= stack.nodalDamageLocal[a];
      qp_grad_damage += temp;
    }

    for( localIndex a = 0; a < numNodesPerElem; ++a )
    {
      if( m_localDissipationOption == 1 )
      {
        stack.localResidual[ a ] += m_detJ[k][q] * ( N[a] * (m_lengthScale * D - 3 * m_Gc / 16 )/ m_Gc -
                                                     0.375*pow( m_lengthScale, 2 ) * LvArray::tensorOps::AiBi< 3 >( qp_grad_damage, m_dNdX[k][q][a] ) -
                                                     m_lengthScale * D/m_Gc * N[a] * qp_damage
                                                     );
      }
      else
      {
        stack.localResidual[ a ] += m_detJ[k][q] * ( N[a] * (2 * m_lengthScale) * strainEnergyDensity / m_Gc -
                                                     ( pow( m_lengthScale, 2 ) * LvArray::tensorOps::AiBi< 3 >( qp_grad_damage, m_dNdX[k][q][a] ) +
                                                       N[a] * qp_damage * (1 + 2 * m_lengthScale*strainEnergyDensity/m_Gc)
                                                     )
                                                     );
      }
      for( localIndex b = 0; b < numNodesPerElem; ++b )
      {
        if( m_localDissipationOption == 1 )
        {
          stack.localJacobian[ a ][ b ] -= m_detJ[k][q] *
                                           (0.375*pow( m_lengthScale, 2 ) * LvArray::tensorOps::AiBi< 3 >( m_dNdX[k][q][a], m_dNdX[k][q][b] ) +
                                            (m_lengthScale * D/m_Gc) * N[a] * N[b]);
        }
        else
        {
          stack.localJacobian[ a ][ b ] -= m_detJ[k][q] *
                                           ( pow( m_lengthScale, 2 ) * LvArray::tensorOps::AiBi< 3 >( m_dNdX[k][q][a], m_dNdX[k][q][b] ) +
                                             N[a] * N[b] * (1 + 2 * m_lengthScale*strainEnergyDensity/m_Gc )
                                           );
        }
      }
    }
  }

  /**
   * @copydoc geosx::finiteElement::ImplicitKernelBase::complete
   *
   * Form element residual from the fully formed element Jacobian dotted with
   * the primary field and map the element local Jacobian/Residual to the
   * global matrix/vector.
   */
  GEOSX_HOST_DEVICE
  GEOSX_FORCE_INLINE
  real64 complete( localIndex const k,
                   StackVariables & stack ) const
  {
    GEOSX_UNUSED_VAR( k );
    real64 maxForce = 0;

    for( int a = 0; a < numNodesPerElem; ++a )
    {
      localIndex const dof = LvArray::integerConversion< localIndex >( stack.localRowDofIndex[ a ] - m_dofRankOffset );
      if( dof < 0 || dof >= m_matrix.numRows() ) continue;
      m_matrix.template addToRowBinarySearchUnsorted< parallelDeviceAtomic >( dof,
                                                                              stack.localColDofIndex,
                                                                              stack.localJacobian[ a ],
                                                                              numNodesPerElem );

      RAJA::atomicAdd< parallelDeviceAtomic >( &m_rhs[ dof ], stack.localResidual[ a ] );
      maxForce = fmax( maxForce, fabs( stack.localResidual[ a ] ) );
    }

    return maxForce;
  }



protected:
  /// The global primary field array.
  arrayView1d< real64 const > const m_nodalDamage;

  /// The global shape function derivatives array.
  arrayView4d< real64 const > const m_dNdX;

  /// The global determinant of the parent/physical Jacobian.
  arrayView2d< real64 const > const m_detJ;

  real64 const m_Gc;
  real64 const m_lengthScale;
  int const m_localDissipationOption;

};


} // namespace geosx

#include "finiteElement/kernelInterface/SparsityKernelBase.hpp"

#endif // GEOSX_PHYSICSSOLVERS_SIMPLEPDE_PHASEFIELDDAMAGEKERNELS_HPP_
