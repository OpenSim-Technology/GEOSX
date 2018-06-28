/*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * Copyright (c) 2018, Lawrence Livermore National Security, LLC.
 *
 * Produced at the Lawrence Livermore National Laboratory
 *
 * LLNL-CODE-746361
 *
 * All rights reserved. See COPYRIGHT for details.
 *
 * This file is part of the GEOSX Simulation Framework.
 *
 * GEOSX is a free software; you can redistrubute it and/or modify it under
 * the terms of the GNU Lesser General Public Liscense (as published by the
 * Free Software Foundation) version 2.1 dated February 1999.
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */

//FUNCTION_BEGIN_PARSE
virtual_int
CohesiveZoneBase::UpdateCohesiveZone( const localIndex index,
                                      const R1Tensor& gap,
                                      const R1Tensor& N,
                                      const std::pair< ElementRegionT*, localIndex >& elem0,
                                      const std::pair< ElementRegionT*, localIndex >& elem1,
                                      R1Tensor& traction,
                                      R2Tensor& stiffness )
{
  return 1;
}
