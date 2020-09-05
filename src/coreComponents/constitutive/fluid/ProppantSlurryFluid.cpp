/*
 * ------------------------------------------------------------------------------------------------------------
 * SPDX-License-Identifier: LGPL-2.1-only
 *
 * Copyright (c) 2018-2019 Lawrence Livermore National Security LLC
 * Copyright (c) 2018-2019 The Board of Trustees of the Leland Stanford Junior University
 * Copyright (c) 2018-2019 Total, S.A
 * Copyright (c) 2019-     GEOSX Contributors
 * All right reserved
 *
 * See top level LICENSE, COPYRIGHT, CONTRIBUTORS, NOTICE, and ACKNOWLEDGEMENTS files for details.
 * ------------------------------------------------------------------------------------------------------------
 */

/**
 * @file ProppantSlurryFluid.cpp
 */

#include "ProppantSlurryFluid.hpp"

namespace geosx
{

using namespace dataRepository;

namespace constitutive
{

ProppantSlurryFluid::ProppantSlurryFluid( std::string const & name, Group * const parent ):
  SlurryFluidBase( name, parent )
{
  registerWrapper( viewKeyStruct::compressibilityString, &m_compressibility )->
    setApplyDefaultValue( 0.0 )->
    setInputFlag( InputFlags::OPTIONAL )->
    setDescription( "Fluid compressibility" );

  registerWrapper( viewKeyStruct::referenceProppantDensityString, &m_referenceProppantDensity )->
    setApplyDefaultValue( 1400.0 )->
    setInputFlag( InputFlags::OPTIONAL )->
    setDescription( "Reference proppant density" );

  registerWrapper( viewKeyStruct::referencePressureString, &m_referencePressure )->
    setApplyDefaultValue( 1e5 )->
    setInputFlag( InputFlags::OPTIONAL )->
    setDescription( "Reference pressure" );

  registerWrapper( viewKeyStruct::referenceDensityString, &m_referenceDensity )->
    setApplyDefaultValue( 1000.0 )->
    setInputFlag( InputFlags::OPTIONAL )->
    setDescription( "Reference fluid density" );

  registerWrapper( viewKeyStruct::referenceViscosityString, &m_referenceViscosity )->
    setApplyDefaultValue( 0.001 )->
    setInputFlag( InputFlags::OPTIONAL )->
    setDescription( "Reference fluid viscosity" );

  registerWrapper( viewKeyStruct::maxProppantConcentrationString, &m_maxProppantConcentration )->
    setApplyDefaultValue( 0.6 )->
    setInputFlag( InputFlags::OPTIONAL )->
    setDescription( "Maximum proppant concentration" );

}

ProppantSlurryFluid::~ProppantSlurryFluid() = default;

void ProppantSlurryFluid::allocateConstitutiveData( dataRepository::Group * const parent,
                                                    localIndex const numConstitutivePointsPerParentIndex )
{
  SlurryFluidBase::allocateConstitutiveData( parent, numConstitutivePointsPerParentIndex );

  m_density.setValues< serialPolicy >( m_referenceDensity );
  m_viscosity.setValues< serialPolicy >( m_referenceViscosity );
}


void ProppantSlurryFluid::PostProcessInput()
{
  SlurryFluidBase::PostProcessInput();

  GEOSX_ERROR_IF_LT_MSG( m_compressibility, 0.0,
                         getName() << ": invalid value of " << viewKeyStruct::compressibilityString );

  GEOSX_ERROR_IF_LE_MSG( m_referenceDensity, 0.0,
                         getName() << ": invalid value of " << viewKeyStruct::referenceDensityString );

  GEOSX_ERROR_IF_LT_MSG( m_referenceViscosity, 0.0,
                         getName() << ": invalid value of " << viewKeyStruct::referenceViscosityString );

  GEOSX_ERROR_IF_LE_MSG( m_maxProppantConcentration, 0.0,
                         getName() << ": invalid value of " << viewKeyStruct::maxProppantConcentrationString );

  GEOSX_ERROR_IF_GT_MSG( m_maxProppantConcentration, 1.0,
                         getName() << ": invalid value of " << viewKeyStruct::maxProppantConcentrationString );
}

ProppantSlurryFluid::KernelWrapper
ProppantSlurryFluid::createKernelWrapper()
{
  return KernelWrapper( m_compressibility,
                        m_referenceProppantDensity,
                        m_referencePressure,
                        m_referenceDensity,
                        m_referenceViscosity,
                        m_maxProppantConcentration,
                        m_defaultDensity.toViewConst(),
                        m_defaultCompressibility.toViewConst(),
                        m_defaultViscosity.toViewConst(),
                        m_nIndices.toViewConst(),
                        m_Ks.toViewConst(),
                        m_isNewtonianFluid,
                        m_density.toView(),
                        m_dDens_dPres.toView(),
                        m_dDens_dProppantConc.toView(),
                        m_dDens_dCompConc.toView(),
                        m_componentDensity.toView(),
                        m_dCompDens_dPres.toView(),
                        m_dCompDens_dCompConc.toView(),
                        m_fluidDensity.toView(),
                        m_dFluidDens_dPres.toView(),
                        m_dFluidDens_dCompConc.toView(),
                        m_fluidViscosity.toView(),
                        m_dFluidVisc_dPres.toView(),
                        m_dFluidVisc_dCompConc.toView(),
                        m_viscosity.toView(),
                        m_dVisc_dPres.toView(),
                        m_dVisc_dProppantConc.toView(),
                        m_dVisc_dCompConc.toView() );
}


REGISTER_CATALOG_ENTRY( ConstitutiveBase, ProppantSlurryFluid, std::string const &, Group * const )

} /* namespace constitutive */

} /* namespace geosx */
