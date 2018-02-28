/**
 * Copyright (c) 2018.
 * Author: Frederic Suter - CNRS / IN2P3 Computing Center
 *
 * This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later
 * version.
 */

#ifndef WRENCH_VIP_STANDARD_JOB_SCHEDULER_H
#define WRENCH_VIP_STANDARD_JOB_SCHEDULER_H

#include <wrench-dev.h>

namespace wrench {

class VipStandardJobScheduler : public StandardJobScheduler {
  FileRegistryService* file_registry_service;

public:
  VipStandardJobScheduler(FileRegistryService* file_registry_service) : file_registry_service(file_registry_service) {}

  void scheduleTasks(const std::set<ComputeService*>& compute_services,
                     const std::map<std::string, std::vector<WorkflowTask*>>& ready_tasks) override;
};
}

#endif
