/**
 * Copyright (c) 2018.
 * Author: Frederic Suter - CNRS / IN2P3 Computing Center
 *
 * This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later
 * version.
 */

#ifndef WRENCH_VIP_PILOT_JOB_SCHEDULER_H
#define WRENCH_VIP_PILOT_JOB_SCHEDULER_H

#include <wrench-dev.h>

namespace wrench {

class VipPilotJobScheduler : public PilotJobScheduler {
  unsigned long how_many = 0;

public:
  VipPilotJobScheduler()  = default;
  ~VipPilotJobScheduler() = default;

  void scheduleNPilotJobs(unsigned long how_many, const std::set<ComputeService*>& compute_services);
  void schedulePilotJobs(const std::set<ComputeService*>& compute_services);
};
}

#endif
