/**
 * Copyright (c) 2018.
 * Author: Frederic Suter - CNRS / IN2P3 Computing Center
 *
 * This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later
 * version.
 */

#include <iostream>
#include <wrench.h>

#include "VipPilotJobScheduler.hpp"
#include "VipStandardJobScheduler.hpp"
#include "VipWMS.hpp"

int main(int argc, char** argv)
{
  wrench::Simulation simulation;

  simulation.init(&argc, argv);

  if (argc != 3) {
    std::cerr << "Usage: " << argv[0] << " <xml platform file> <workflow file>" << std::endl;
    exit(1);
  }

  char* platform_file = argv[1];
  char* workflow_file = argv[2];

  /* Reading and parsing the workflow description file to create a wrench::Workflow object */
  std::cerr << "Loading workflow..." << std::endl;
  wrench::Workflow workflow;
  workflow.loadFromJSON(workflow_file);
  std::cerr << "The workflow has " << workflow.getNumberOfTasks() << " tasks " << std::endl;
  std::cerr.flush();

  /* Reading and parsing the platform description file to instantiate a simulated platform */
  std::cerr << "Instantiating SimGrid platform..." << std::endl;
  simulation.instantiatePlatform(platform_file);

  /* Get a vector of all the hosts in the simulated platform */
  std::vector<std::string> hostname_list = simulation.getHostnameList();

  /* List of known storage elements in the particular platform file used for that first test */
  std::vector<std::string> storage_elements = {"bohr3226.tier2.hep.manchester.ac.uk", "ccsrm02.in2p3.fr",
                                               "dc2-grid-64.brunel.ac.uk", "marsedpm.in2p3.fr", "tbn18.nikhef.nl"};

  /* Instantiate a storage service on each storage element, and store them in a vector for further usage*/
  std::vector<wrench::StorageService*> storage_services;
  for (auto s : storage_elements) {
    std::cerr << "Instantiating a SimpleStorageService on " << s << "..." << std::endl;
    storage_services.push_back(simulation.add(new wrench::SimpleStorageService(s, 1.0e12))); // 1TB
  }
  wrench::StorageService* x = storage_services.at(4);

  /* Construct (very specific) lists of hosts on which to run tasks */
  std::vector<std::string> sara_matrix_hosts           = {"am-9027.gina.sara.nl"};
  std::vector<std::string> uki_northgrid_man_hep_hosts = {
      "wn1904200.tier2.hep.manchester.ac.uk", "wn1905300.tier2.hep.manchester.ac.uk",
      "wn2206251.tier2.hep.manchester.ac.uk", "wn2206291.tier2.hep.manchester.ac.uk"};

  /* Instantiate two batch services, one per site to be started on some host in the simulation platform. */
  std::string wms_host = "vip.creatis.insa-lyon.fr";

  wrench::ComputeService* sara_matrix_batch_service =
      new wrench::BatchService(sara_matrix_hosts.front(), true, true, sara_matrix_hosts,
                               storage_services.at(4), // tbn18.nikhef.nl
                               {{wrench::BatchServiceProperty::STOP_DAEMON_MESSAGE_PAYLOAD, "2048"}});

  wrench::ComputeService* uki_northgrid_man_hep_service =
      new wrench::BatchService(uki_northgrid_man_hep_hosts.front(), true, true, uki_northgrid_man_hep_hosts,
                               storage_services.at(0), // bohr3226.tier2
                               {{wrench::BatchServiceProperty::STOP_DAEMON_MESSAGE_PAYLOAD, "2048"}});

  /* Add the compute services to the simulation, catching a possible exception */
  try {
    simulation.add(sara_matrix_batch_service);
    simulation.add(uki_northgrid_man_hep_service);
  } catch (std::invalid_argument& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    std::exit(1);
  }

  /* Create a set of compute services that will be used by the WMS */
  std::set<wrench::ComputeService*> compute_services;
  compute_services.insert(sara_matrix_batch_service);
  compute_services.insert(uki_northgrid_man_hep_service);

  /* Instantiate a file registry service to be started on some host. This service is essentially a replica catalog that
   * stores <file , storage service> pairs so that any service, in particular a WMS, can discover where workflow files
   * are stored.
   */
  std::string file_registry_service_host = "lfc-biomed.in2p3.fr";
  std::cerr << "Instantiating a FileRegistryService on " << file_registry_service_host << "..." << std::endl;

  wrench::FileRegistryService* file_registry_service = new wrench::FileRegistryService(file_registry_service_host);
  simulation.setFileRegistryService(file_registry_service);

  /* Instantiate a WMS, to be started on some host (wms_host), which is responsible for executing the workflow */
  std::cerr << "Instantiating a WMS on " << wms_host << "..." << std::endl;
  wrench::WMS* wms = simulation.add(new wrench::VipWMS(
      std::unique_ptr<wrench::VipStandardJobScheduler>(new wrench::VipStandardJobScheduler(file_registry_service)),
      std::unique_ptr<wrench::VipPilotJobScheduler>(new wrench::VipPilotJobScheduler(&workflow)), compute_services,
      {storage_services.begin(), storage_services.end()}, file_registry_service, wms_host));

  wms->addWorkflow(&workflow, 0);

  /* It is necessary to store, or "stage", input files for the first task(s) of the workflow on some storage service,
   * so that workflow execution can be initiated. The getInputFiles() method of the Workflow class returns the set of
   * all workflow files that are not generated by workflow tasks, and thus are only input files. These files are then
   * staged on the storage service.
   */
  std::cerr << "Staging input files..." << std::endl;
  std::map<std::string, wrench::WorkflowFile*> input_files = workflow.getInputFiles();
  for (auto f : input_files)
    std::cout << f.second->getId() << std::endl;
  std::cout.flush();

  try {
    simulation.stageFile(input_files.at("gate.sh.tar.gz"), storage_services.at(1));
    simulation.stageFile(input_files.at("gate.sh.tar.gz"), storage_services.at(3));
    simulation.stageFile(input_files.at("gate.sh.tar.gz"), storage_services.at(4));
    simulation.stageFile(input_files.at("release_Gate7.1_all.tar.gz"), storage_services.at(0));
    simulation.stageFile(input_files.at("file-66406575341200.zip"), storage_services.at(2));
    simulation.stageFile(input_files.at("merge.sh.tar.gz"), storage_services.at(4));
  } catch (std::runtime_error& e) {
    std::cerr << "Exception: " << e.what() << std::endl;
    return 0;
  }

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