/**
 * Copyright (c) 2018.
 * Author: Frederic Suter - CNRS / IN2P3 Computing Center
 *
 * This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later
 * version.
 */

#include "VipPilotJobScheduler.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(vip_pilot_job_scheduler, "Log category for VIP Scheduler");

namespace wrench {

void VipPilotJobScheduler::schedulePilotJobs(const std::set<ComputeService*>& compute_services)
{
  wrench::ComputeService* big_cluster;
  wrench::ComputeService* small_cluster;
  for (auto s : compute_services)
    if (s->getNumHosts() > 1)
      big_cluster = s;
    else
      small_cluster = s;

  std::vector<WorkflowJob*> pilots;

  for (unsigned int i = 0; i < this->workflow->getNumberOfTasks(); i++) {
    pilots.push_back(static_cast<WorkflowJob*>(this->job_manager->createPilotJob(workflow, 1, 1, 0.0, 36000)));
  }
  std::map<std::string, std::string> batch_job_args;
  batch_job_args["-N"] = "1";
  batch_job_args["-t"] = "600"; // time in minutes
  batch_job_args["-c"] = "1";   // number of cores per node

  for (auto it = pilots.begin() + 1; it != pilots.end(); ++it)
    this->job_manager->submitJob(*it, big_cluster, batch_job_args);

  this->job_manager->submitJob(pilots.front(), small_cluster, batch_job_args);
}
}
