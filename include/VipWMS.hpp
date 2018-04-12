/**
 * Copyright (c) 2018.
 * Author: Frederic Suter - CNRS / IN2P3 Computing Center
 *
 * This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later
 * version.
 */

#ifndef WRENCH_VIP_WMS_H
#define WRENCH_VIP_WMS_H

#include <wrench-dev.h>

namespace wrench {

class VipWMS : public WMS {

public:
  VipWMS(std::unique_ptr<StandardJobScheduler> StandardJobscheduler,
         std::unique_ptr<PilotJobScheduler> PilotJobscheduler, const std::set<ComputeService*>& compute_services,
         const std::set<StorageService*>& storage_services, wrench::FileRegistryService* file_registry_service,
         const std::string& hostname);
  ~VipWMS() { delete available_compute_resources; }

protected:
  void processEventStandardJobFailure(std::unique_ptr<StandardJobFailedEvent>);
  void processEventPilotJobStart(std::unique_ptr<PilotJobStartedEvent>);
  std::deque<ComputeService*>* getAvailableComputeResources() { return available_compute_resources; }

private:
  int main() override;

  std::shared_ptr<JobManager> job_manager;
  std::deque<ComputeService*>* available_compute_resources;
  /** @brief Whether the workflow execution should be aborted */
  bool abort = false;
};
}
#endif
