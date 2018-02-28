/**
 * Copyright (c) 2018.
 * Author: Frederic Suter - CNRS / IN2P3 Computing Center
 *
 * This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later
 * version.
 */

#include "VipStandardJobScheduler.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(vip_standard_job_scheduler, "Log category for VIP Scheduler");

namespace wrench {

void VipStandardJobScheduler::scheduleTasks(const std::set<ComputeService*>& compute_services,
                                            const std::map<std::string, std::vector<WorkflowTask*>>& ready_tasks)
{

  WRENCH_INFO("There are %ld ready tasks to schedule", ready_tasks.size());

  auto task = ready_tasks.begin();
  std::map<wrench::WorkflowFile*, wrench::StorageService*> locations;
  std::set<wrench::WorkflowFile*> input_files = task->second.front()->getInputFiles();
  for (auto f : input_files) {
    std::set<StorageService*> replica_locations = file_registry_service->lookupEntry(f);
    locations.insert({f, *(replica_locations.begin())});
  }

  WorkflowJob* job = (WorkflowJob*)this->job_manager->createStandardJob(task->second, locations);
  this->job_manager->submitJob(job, *(compute_services.begin()));

  // FIXME: this should be here, but we have to register files manually for now
  std::set<wrench::WorkflowFile*> output_files = task->second.front()->getOutputFiles();
  ComputeService* c                            = *(compute_services.begin());
  for (auto f : output_files) {
    file_registry_service->addEntry(f, c->getDefaultStorageService());
  }

  WRENCH_INFO("Done with scheduling tasks as standard jobs");
}
}
