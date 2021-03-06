/**
 * Copyright (c) 2018.
 * Author: Frederic Suter - CNRS / IN2P3 Computing Center
 *
 * This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later
 * version.
 */

#include <boost/algorithm/string.hpp>
#include <fstream>
#include <iostream>
#include <wrench.h>

#include "VipPilotJobScheduler.hpp"
#include "VipStandardJobScheduler.hpp"
#include "VipWMS.hpp"

// TODO move these functions in a utils.cpp file
static wrench::StorageService* getStorageServiceByname(std::set<wrench::StorageService*> service_set, std::string name)
{
  return *(std::find_if(service_set.begin(), service_set.end(),
                        [&name](wrench::StorageService* s) -> bool { return s->getHostname() == name; }));
}

static void stageFilesFromCsv(wrench::Simulation* simulation, wrench::Workflow* workflow,
                              std::set<wrench::StorageService*> storage_services, std::string filename)
{
  std::string line;
  std::ifstream* fs = new std::ifstream();
  fs->open(filename.c_str(), std::ifstream::in);
  do {
    std::getline(*fs, line);
    boost::trim(line);
    if (line.length() > 0) {
      std::vector<std::string> tokens;
      boost::split(tokens, line, boost::is_any_of(","), boost::token_compress_on);

      std::vector<std::string> path_elms;
      boost::split(path_elms, tokens[0], boost::is_any_of("/"), boost::token_compress_on);
      std::string name = path_elms.back();

      unsigned long long size = std::stoull(tokens[1]);
      std::vector<std::string> locations;

      boost::split(locations, tokens[2], boost::is_any_of(":"), boost::token_compress_on);

      for (auto loc : locations) {
        try {
          simulation->stageFile(workflow->getWorkflowFileByID(name), getStorageServiceByname(storage_services, loc));
        } catch (std::runtime_error& e) {
          std::cerr << "Exception: " << e.what() << std::endl;
        }
      }
    }
  } while (not fs->eof());

  delete fs;
}

int main(int argc, char** argv)
{
  wrench::Simulation simulation;

  simulation.init(&argc, argv);

  if (argc != 4) {
    std::cerr << "Usage: " << argv[0] << " <xml platform file> <workflow file> <catalog file>" << std::endl;
    exit(1);
  }

  /* Reading and parsing the platform description file to instantiate a simulated platform */
  std::cerr << "Instantiating SimGrid platform..." << std::endl;
  simulation.instantiatePlatform(argv[1]);

  /* List of known storage elements in the particular platform file used for that first test */
  std::vector<std::string> storage_elements = {"tbn18.nikhef.nl", "bohr3226.tier2.hep.manchester.ac.uk",
                                               "ccsrm02.in2p3.fr", "dc2-grid-64.brunel.ac.uk", "marsedpm.in2p3.fr"};

  /* Instantiate a storage service on each storage element, and store them in a vector for further usage*/
  std::set<wrench::StorageService*> storage_services;
  for (auto s : storage_elements) {
    std::cerr << "Instantiating a SimpleStorageService on " << s << "..." << std::endl;
    storage_services.insert(simulation.add(new wrench::SimpleStorageService(s, 1.0e12))); // 1TB
  }

  std::map<std::string, std::vector<std::string>> hostname_by_cluster_list = simulation.getHostnameListByCluster();

  /* Create a set of compute services that will be used by the WMS */
  std::set<wrench::ComputeService*> compute_services;

  for (auto c : hostname_by_cluster_list) {
    const char* close_SE_name        = simgrid::s4u::Host::by_name(c.second.front())->getProperty("closeSE");
    wrench::StorageService* close_SE = getStorageServiceByname(storage_services, close_SE_name);

    wrench::ComputeService* batch_service =
        new wrench::BatchService(c.second.front(), true, true, c.second, close_SE,
                                 {{wrench::BatchServiceProperty::STOP_DAEMON_MESSAGE_PAYLOAD, "2048"}});
    /* Add the compute services to the simulation, catching a possible exception */
    try {
      compute_services.insert(simulation.add(batch_service));
    } catch (std::invalid_argument& e) {
      std::cerr << "Error: " << e.what() << std::endl;
      std::exit(1);
    }
  }

  /* Instantiate a file registry service to be started on some host. This service is essentially a replica catalog that
   * stores <file , storage service> pairs so that any service, in particular a WMS, can discover where workflow files
   * are stored.
   */
  std::string DefaultLFC = "lfc-biomed.in2p3.fr";
  std::cerr << "Instantiating a FileRegistryService on " << DefaultLFC << "..." << std::endl;

  wrench::FileRegistryService* file_registry_service = simulation.add(new wrench::FileRegistryService(DefaultLFC));

  /* Instantiate a WMS, to be started on some host (wms_host), which is responsible for executing the workflow */
  std::string VIPServer = "vip.creatis.insa-lyon.fr";
  std::cerr << "Instantiating a WMS on " << VIPServer << "..." << std::endl;
  wrench::WMS* wms = simulation.add(new wrench::VipWMS(
      std::unique_ptr<wrench::VipStandardJobScheduler>(new wrench::VipStandardJobScheduler(file_registry_service)),
      std::unique_ptr<wrench::VipPilotJobScheduler>(new wrench::VipPilotJobScheduler()), compute_services,
      storage_services, file_registry_service, VIPServer));

  /* Reading and parsing the workflow description file to create a wrench::Workflow object */
  std::cerr << "Loading workflow..." << std::endl;
  wrench::Workflow workflow;
  workflow.loadFromJSON(argv[2], "1f");
  std::cerr << "The workflow has " << workflow.getNumberOfTasks() << " tasks " << std::endl;
  std::cerr.flush();

  wms->addWorkflow(&workflow, 0);

  /* It is necessary to store, or "stage", input files for the first task(s) of the workflow on some storage service,
   * so that workflow execution can be initiated. The getInputFiles() method of the Workflow class returns the set of
   * all workflow files that are not generated by workflow tasks, and thus are only input files. These files are then
   * staged on the storage service.
   */
  std::cerr << "Staging input files..." << std::endl;
  stageFilesFromCsv(&simulation, &workflow, storage_services, argv[3]);

  /* Launch the simulation. This call only returns when the simulation is complete. */
  std::cerr << "Launching the Simulation..." << std::endl;
  try {
    simulation.launch();
  } catch (std::runtime_error& e) {
    std::cerr << "Exception: " << e.what() << std::endl;
    return 0;
  }
  std::cerr << "Simulation done!" << std::endl;

  /* Simulation results can be examined via simulation.output, which provides access to traces of events. In the code
   * below, we retrieve the trace of all task completion events, print how many such events there are, and print some
   * information for the first such event.
   */
  std::vector<wrench::SimulationTimestamp<wrench::SimulationTimestampTaskCompletion>*> trace;
  trace = simulation.output.getTrace<wrench::SimulationTimestampTaskCompletion>();
  std::cerr << "Number of entries in TaskCompletion trace: " << trace.size() << std::endl;
  std::cerr << "Task in first trace entry: " << trace[0]->getContent()->getTask()->getId() << std::endl;

  return 0;
}
