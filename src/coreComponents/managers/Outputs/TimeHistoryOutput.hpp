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
 * @file TimeHistoryOutput.hpp
 */

#ifndef GEOSX_TIME_HISTORY_OUTPUT_HPP_
#define GEOSX_TIME_HISTORY_OUTPUT_HPP_

#include "OutputBase.hpp"
#include "managers/TimeHistory/TimeHistoryCollection.hpp"
#include "managers/TimeHistory/HistoryIO.hpp"
#include "fileIO/timeHistory/TimeHistHDF.hpp"

#include "LvArray/src/Array.hpp" // just for collector

namespace geosx
{

/**
 * @class TimeHistoryOutput
 *
 * A class for creating time history output
 */
class TimeHistoryOutput : public OutputBase
{
public:
  /// @copydoc geosx::dataRepository::Group::Group(std::string const & name, Group * const parent)
  TimeHistoryOutput( string const & name,
                     Group * const parent );

  /// Destructor
  virtual ~TimeHistoryOutput() override
  { }

  /**
   * @brief Catalog name interface
   * @return This type's catalog name
   */
  static string CatalogName() { return "TimeHistory"; }

  /**
   * @brief Perform initalization after all subgroups have been initialized.
   *        Check for existing files and data spaces/sets on restart, else create the
   *        file and data spaces/sets and output any set index metatata for the time
   *        history.
   * @param group The problem manager cast to a group.
   * @note There are operations in this function that are collective on the GEOSX comm.
   */
  virtual void InitializePostSubGroups( Group * const group ) override;

  /**
   * @brief Writes out a time history file.
   * @copydoc EventBase::Execute()
   */
  virtual void Execute( real64 const time_n,
                        real64 const dt,
                        integer const cycleNumber,
                        integer const eventCounter,
                        real64 const eventProgress,
                        dataRepository::Group * domain ) override;
  /**
   * @brief Writes out a time history file at the end of the simulation.
   * @copydoc ExecutableGroup::Cleanup()
   */
  virtual void Cleanup( real64 const time_n,
                        integer const cycleNumber,
                        integer const eventCounter,
                        real64 const eventProgress,
                        dataRepository::Group * domain ) override;

  /// @cond DO_NOT_DOCUMENT
  struct viewKeys
  {
    static constexpr auto timeHistoryOutputTarget = "sources";
    static constexpr auto timeHistoryOutputFilename = "filename";
    static constexpr auto timeHistoryOutputFormat = "format";
    static constexpr auto timeHistoryRestart = "restart";
  } timeHistoryOutputViewKeys;
  /// @endcond

private:

  /**
   * @brief Initialize a time history collector to write to an MPI comm-specific file collectively.
   * @param group The ProblemManager cast to a Group
   * @param collector The HistoryCollector to intialize
   */
  void initCollectorParallel( ProblemManager & problemManager, HistoryCollection * collector );

  /// The paths of the collectors to collect history from.
  string_array m_collectorPaths;
  /// The file format of the time history file.
  string m_format;
  /// The filename of the time history file.
  string m_filename;
  /// The discrete number of time history states expected to be written to the file
  integer m_recordCount;
  /// The buffered time history output objects for each collector to collect data into and to use to configure/write to file.
  std::vector< std::unique_ptr< BufferedHistoryIO > > m_io;
};
}

#endif
