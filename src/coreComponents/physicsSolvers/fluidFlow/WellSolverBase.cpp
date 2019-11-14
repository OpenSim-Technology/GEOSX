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
 * @file WellSolverBase.cpp
 */

#include "WellSolverBase.hpp"

#include "managers/DomainPartition.hpp"
#include "wells/WellControls.hpp"
#include "wells/WellElementRegion.hpp"
#include "wells/WellElementSubRegion.hpp"
#include "wells/PerforationData.hpp"

namespace geosx
{

using namespace dataRepository;
using namespace constitutive;
  
WellSolverBase::WellSolverBase( std::string const & name,
                                Group * const parent )
  : SolverBase( name, parent ),
    m_flowSolverName(""),
    m_gravityFlag(1),
    m_fluidName(),
    m_resFluidIndex(),
    m_numDofPerWellElement(0),
    m_numDofPerResElement(0)
{
  registerWrapper( viewKeyStruct::gravityFlagString, &m_gravityFlag, false )->
    setApplyDefaultValue(1)->
    setInputFlag(InputFlags::REQUIRED)->
    setDescription("Flag that enables/disables gravity");

  this->registerWrapper( viewKeyStruct::fluidNameString,  &m_fluidName,  false )->
    setInputFlag(InputFlags::REQUIRED)->
    setDescription("Name of fluid constitutive object to use for this solver.");

  this->registerWrapper( viewKeyStruct::resFluidIndexString, &m_resFluidIndex, false );

}

Group * WellSolverBase::CreateChild( string const & childKey, string const & childName )
{
  Group * rval = nullptr;

  if( childKey == keys::wellControls )
  {
    rval = RegisterGroup<WellControls>( childName );
  }
  else
  {
    SolverBase::CreateChild( childKey, childName );
  }
  return rval;
}

WellSolverBase::~WellSolverBase() = default;

void WellSolverBase::RegisterDataOnMesh( Group * const meshBodies )
{
  SolverBase::RegisterDataOnMesh( meshBodies );

  MeshLevel * const meshLevel = meshBodies->GetGroup<MeshBody>(0)->getMeshLevel(0);
  ElementRegionManager * const elemManager = meshLevel->getElemManager();

  // loop over the wells
  elemManager->forElementSubRegions<WellElementSubRegion>( [&]( WellElementSubRegion * const subRegion )
  {
    subRegion->registerWrapper<array1d<real64>>( viewKeyStruct::gravityDepthString );

    PerforationData * const perforationData = subRegion->GetPerforationData();
    perforationData->registerWrapper<array1d<real64>>( viewKeyStruct::gravityDepthString );
  });
}

void WellSolverBase::ImplicitStepSetup( real64 const & time_n,
                                        real64 const & GEOSX_UNUSED_ARG( dt ),
                                        DomainPartition * const domain,
                                        DofManager & GEOSX_UNUSED_ARG( dofManager ),
                                        ParallelMatrix & GEOSX_UNUSED_ARG( matrix ),
                                        ParallelVector & GEOSX_UNUSED_ARG( rhs ),
                                        ParallelVector & GEOSX_UNUSED_ARG( solution ) )
{
  // bind the stored reservoir views to the current domain
  ResetViews( domain );

  FlowSolverBase const * const flowSolver = getParent()->GetGroup<FlowSolverBase>( GetFlowSolverName() );
  m_numDofPerResElement = flowSolver->numDofPerCell();

  // Initialize the primary and secondary variables for the first time step
  if (time_n <= 0.0)
  {
    InitializeWells( domain );
  }

  // set deltas to zero and recompute dependent quantities
  ResetStateToBeginningOfStep( domain );
}

void WellSolverBase::AssembleSystem( real64 const time,
                                     real64 const dt,
                                     DomainPartition * const domain,
                                     DofManager const & dofManager,
                                     ParallelMatrix & matrix,
                                     ParallelVector & rhs )
{ 

  // first deal with the well control and switch if necessary
  CheckWellControlSwitch( domain );

  // then assemble the mass balance equations
  AssembleFluxTerms( time, dt, domain, &dofManager, &matrix, &rhs );
  AssemblePerforationTerms( time, dt, domain, &dofManager, &matrix, &rhs );

  // then assemble the volume balance equations
  AssembleVolumeBalanceTerms( time, dt, domain, &dofManager, &matrix, &rhs );

  // then assemble the pressure relations between well elements
  FormPressureRelations( domain, &dofManager, &matrix, &rhs );

  // finally assemble the well control equation
  FormControlEquation( domain, &dofManager, &matrix, &rhs );

  // Log messages for verbosity >= 2
  VERBOSE_LOG_RANK_0(2, "After WellSolverBase::AssembleSystem" );
  VERBOSE_LOG_RANK_0(2, "\nJacobian:\n" << matrix );
  VERBOSE_LOG_RANK_0(2, "\nResidual:\n" << rhs );
}

void WellSolverBase::UpdateStateAll( DomainPartition * const domain )
{

  MeshLevel * const meshLevel = domain->getMeshBodies()->GetGroup<MeshBody>(0)->getMeshLevel(0);
  ElementRegionManager * const elemManager = meshLevel->getElemManager();

  elemManager->forElementSubRegions<WellElementSubRegion>( [&]( WellElementSubRegion * const subRegion )
  {
    UpdateState( subRegion );
  });

}
 
void WellSolverBase::InitializePreSubGroups(Group * const rootGroup)
{
  SolverBase::InitializePreSubGroups(rootGroup);

  DomainPartition * domain = rootGroup->GetGroup<DomainPartition>(keys::domain);
  ConstitutiveManager * const cm = domain->getConstitutiveManager();

  ConstitutiveBase const * fluid  = cm->GetConstitutiveRelation<ConstitutiveBase>( m_fluidName );
  GEOS_ERROR_IF( fluid == nullptr, "Fluid model " + m_fluidName + " not found" );

  m_resFluidIndex = fluid->getIndexInParent(); 
}
  
void WellSolverBase::InitializePostInitialConditions_PreSubGroups(Group * const rootGroup)
{
  SolverBase::InitializePostInitialConditions_PreSubGroups(rootGroup);

  DomainPartition * const domain = rootGroup->GetGroup<DomainPartition>(keys::domain);

  // make sure that nextWellElementIndex is up-to-date (will be used in well initialization and assembly)
  MeshLevel * const mesh = domain->getMeshBody( 0 )->getMeshLevel( 0 );
  ElementRegionManager * const elemManager = mesh->getElemManager();

  elemManager->forElementSubRegions<WellElementSubRegion>([&]( WellElementSubRegion * const subRegion )
  {
    subRegion->ReconstructLocalConnectivity();
  });

  // bind the stored reservoir views to the current domain
  ResetViews( domain );
  
  // Precompute solver-specific constant data (e.g. gravity-depth)
  PrecomputeData(domain);
}

void WellSolverBase::PrecomputeData(DomainPartition * const domain)
{
  R1Tensor const & gravityVector = getGravityVector();

  MeshLevel * const meshLevel = domain->getMeshBodies()->GetGroup<MeshBody>(0)->getMeshLevel(0);
  ElementRegionManager * const elemManager = meshLevel->getElemManager();

  // loop over the wells
  elemManager->forElementSubRegions<WellElementSubRegion>([&]( WellElementSubRegion * const subRegion )
  {
    WellControls * const wellControls = GetWellControls( subRegion );

    PerforationData const * const perforationData = subRegion->GetPerforationData();

    arrayView1d<R1Tensor const> const & wellElemLocation = 
      subRegion->getReference<array1d<R1Tensor>>( ElementSubRegionBase::viewKeyStruct::elementCenterString );

    arrayView1d<real64> const & wellElemGravDepth = 
      subRegion->getReference<array1d<real64>>( viewKeyStruct::gravityDepthString );

    arrayView1d<R1Tensor const> const & perfLocation = 
      perforationData->getReference<array1d<R1Tensor>>( PerforationData::viewKeyStruct::locationString );

    arrayView1d<real64> const & perfGravDepth = 
      perforationData->getReference<array1d<real64>>( viewKeyStruct::gravityDepthString );

    for (localIndex iwelem = 0; iwelem < subRegion->size(); ++iwelem)
    {
      // precompute the depth of the well elements
      wellElemGravDepth[iwelem] = Dot( wellElemLocation[iwelem], gravityVector );

    }

    forall_in_range( 0, perforationData->size(), GEOSX_LAMBDA ( localIndex const iperf )
    {
      // precompute the depth of the perforations
      perfGravDepth[iperf] = Dot( perfLocation[iperf], gravityVector );
    });


    // set the first well element of the well    
    if (subRegion->IsLocallyOwned())
    {
 
      localIndex const iwelemControl = subRegion->GetTopWellElementIndex();

      GEOS_ERROR_IF( iwelemControl < 0, 
                     "Invalid well definition: well " << subRegion->getName() 
                  << " has no well head");
  
      // save the index of reference well element (used to enforce constraints) 
      wellControls->SetReferenceWellElementIndex( iwelemControl );
    }
  });
}

void WellSolverBase::ResetViews( DomainPartition * const domain )
{
  MeshLevel * const mesh = domain->getMeshBody( 0 )->getMeshLevel( 0 );
  ElementRegionManager * const elemManager = mesh->getElemManager();

  m_resGravDepth =
    elemManager->ConstructViewAccessor<array1d<real64>, arrayView1d<real64>>( FlowSolverBase::viewKeyStruct::gravityDepthString );
}
  
WellControls * WellSolverBase::GetWellControls( WellElementSubRegion const * const subRegion )
{
  string const & name = subRegion->GetWellControlsName();

  WellControls * wellControls = this->GetGroup<WellControls>( name );
  GEOS_ERROR_IF( wellControls == nullptr, "Well constraint " + name + " not found" ); 

  return wellControls;
}

WellControls const * WellSolverBase::GetWellControls( WellElementSubRegion const * const subRegion ) const
{
  string const & name = subRegion->GetWellControlsName();

  WellControls const * wellControls = this->GetGroup<WellControls>( name );
  GEOS_ERROR_IF( wellControls == nullptr, "Well constraint " + name + " not found" ); 

  return wellControls;
}

  
} // namespace geosx
