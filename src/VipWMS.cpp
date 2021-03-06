/**
 * Copyright (c) 2018.
 * Author: Frederic Suter - CNRS / IN2P3 Computing Center
 *
 * This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later
 * version.
 */

#include "VipWMS.hpp"
#include "VipPilotJobScheduler.hpp"

#include <iostream>

XBT_LOG_NEW_DEFAULT_CATEGORY(vip_wms, "Log category for VIP WMS");

namespace wrench {

VipWMS::VipWMS(std::unique_ptr<StandardJobScheduler> standard_job_scheduler,
               std::unique_ptr<PilotJobScheduler> pilot_job_scheduler,
               const std::set<ComputeService*>& compute_services, const std::set<StorageService*>& storage_services,
               wrench::FileRegistryService* file_registry_service, const std::string& hostname)
    : WMS(std::move(standard_job_scheduler), std::move(pilot_job_scheduler), compute_services, storage_services,
          {} /* no network proximity service */, file_registry_service, hostname, "VIP")
{
  available_compute_resources = new std::deque<ComputeService*>;
}

int VipWMS::main()
{

  TerminalOutput::setThisProcessLoggingColor(COLOR_GREEN);

  // Check whether the WMS has a deferred start time
  checkDeferredStart();

  WRENCH_INFO("About to execute a workflow with %lu tasks", this->workflow->getNumberOfTasks());

  // Create a job manager
  this->job_manager = this->createJobManager();

  // Create a data movement manager
  std::shared_ptr<DataMovementManager> data_movement_manager = this->createDataMovementManager();

  static_cast<VipPilotJobScheduler*>(this->pilot_job_scheduler.get())
      ->scheduleNPilotJobs(this->workflow->getNumberOfTasks(), this->getAvailableComputeServices());

  while (true) {
    // Get the ready tasks
    std::vector<WorkflowTask*> ready_tasks = this->workflow->getReadyTasks();

    // Get the available compute services
    std::deque<ComputeService*>* compute_resources = this->getAvailableComputeResources();
    WRENCH_INFO("There are %lu available resources", compute_resources->size());
    // Run ready tasks with defined scheduler implementation
    if (not ready_tasks.empty() && not compute_resources->empty()) {
      WRENCH_INFO("Scheduling tasks...");
      std::vector<WorkflowTask*> task;
      task.push_back(ready_tasks.front());
      this->standard_job_scheduler->scheduleTasks({compute_resources->front()}, task);

      compute_resources->pop_front();
    }

    // Wait for a workflow execution event, and process it
    try {
      this->waitForAndProcessNextEvent();
    } catch (WorkflowExecutionException& e) {
      WRENCH_INFO("Error while getting next execution event (%s)... ignoring "
                  "and trying again",
                  (e.getCause()->toString().c_str()));
      continue;
    }

    if (this->abort || workflow->isDone()) {
      break;
    }
  }

  WRENCH_INFO("--------------------------------------------------------");
  if (workflow->isDone()) {
    WRENCH_INFO("Workflow execution is complete!");
  } else {
    WRENCH_INFO("Workflow execution is incomplete!");
  }

  WRENCH_INFO("Simple WMS Daemon started on host %s terminating", S4U_Simulation::getHostName().c_str());

  this->job_manager.reset();

  return 0;
}

/**
 * @brief Process a WorkflowExecutionEvent::STANDARD_JOB_FAILURE
 *
 * @param event: a workflow execution event
 */
void VipWMS::processEventStandardJobFailure(std::unique_ptr<StandardJobFailedEvent> event)
{
  WRENCH_INFO("Notified that a standard job has failed (all its tasks are back "
              "in the ready state)");
  WRENCH_INFO("CauseType: %s", event->failure_cause->toString().c_str());
  this->job_manager->forgetJob(event->standard_job);
  WRENCH_INFO("As a SimpleWMS, I abort as soon as there is a failure");
  this->abort = true;
}

void VipWMS::processEventPilotJobStart(std::unique_ptr<PilotJobStartedEvent> event)
{
  this->available_compute_resources->push_back(event->pilot_job->getComputeService());
}
}
