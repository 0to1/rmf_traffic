/*
 * Copyright (C) 2021 Open Source Robotics Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
*/
#ifndef SRC__RMF_TRAFFIC__RESERVATIONS__INTERNAL_PLANNER
#define SRC__RMF_TRAFFIC__RESERVATIONS__INTERNAL_PLANNER
#include <rmf_traffic/reservations/Database.hpp>
#include "../internal_State.hpp"
#include "../internal_ConstraintTracker.hpp"
#include <set>
#include <map>
#include <queue>

namespace rmf_traffic {
namespace reservations {


/*class CostFunction
{
public:
  virtual float cost(
    std::vector<AbstractScheduleOperator>& operations,
    std::shared_ptr<ConstraintTracker>& constraints,
    std::shared_ptr<AbstractScheduleState> original_state) = 0;
};*/

class PushbackScheduleOperator
{
public:
  // Parameters
  std::string resource_name;

  rmf_traffic::Time start_time;
  rmf_traffic::Time desired_time;

  const std::optional<std::shared_ptr<SchedulePatch>>
    apply(
      std::shared_ptr<const AbstractScheduleState> state,
      std::shared_ptr<ConstraintTracker> tracker)
  {
    auto sched = state->get_schedule(resource_name);
    std::vector<Reservation> proposal;
    auto item = sched.lower_bound(start_time);
    auto next_time = desired_time;
    while(item != sched.end() && item->first <= next_time)
    {
      auto proposed_push_back = item->second.propose_new_start_time(next_time);
      auto associated_request =
        tracker->get_associated_reservation(proposed_push_back.reservation_id());
      if(!tracker->satisfies(associated_request.value(), proposed_push_back))
      {
        return std::nullopt;
      }
      auto last_fin_time = proposed_push_back.actual_finish_time();
      proposal.push_back(proposed_push_back);
      if(!last_fin_time.has_value())
      {
        // Infinite resolution
        break;
      }
      next_time = last_fin_time.value();
      item = std::next(item);
    }

    auto final_sched = std::make_shared<SchedulePatch>(state);

    for(auto i = proposal.size() - 1; 0 <= i; i--)
    {
      if(final_sched->update_reservation(proposal[i]))
      {
        return std::nullopt;
      }
    }

    return final_sched;
  }
};


class BringForwardScheduleOperator
{
public:
  // Parameters
  std::string resource_name;

  rmf_traffic::Time start_time;
  rmf_traffic::Time desired_time;

  const std::optional<std::shared_ptr<SchedulePatch>>
    apply(
      std::shared_ptr<const AbstractScheduleState> state,
      std::shared_ptr<ConstraintTracker> tracker)
  {
    auto sched = state->get_schedule(resource_name);
    std::vector<Reservation> proposal;
    auto after = sched.lower_bound(start_time);
    auto item = std::make_reverse_iterator(std::prev(after));
    auto next_time = desired_time;
    while(item != sched.rend() && item->first > next_time)
    {
      auto proposed_push_back = item->second.propose_new_start_time(next_time);
      auto request_id =
        tracker->get_associated_reservation(proposed_push_back.reservation_id());
      if(!tracker->satisfies(request_id.value(), proposed_push_back))
      {
        return std::nullopt;
      }
      auto last_fin_time = proposed_push_back.actual_finish_time();
      proposal.push_back(proposed_push_back);
      if(!last_fin_time.has_value())
      {
        break;
      }
      next_time = last_fin_time.value();
      item = std::prev(item);
    }

    auto final_sched = std::make_shared<SchedulePatch>(state);

    for(auto i = proposal.size() - 1; 0 <= i; i--)
    {
      if(final_sched->update_reservation(proposal[i]))
      {
        return std::nullopt;
      }
    }

    return final_sched;
  }
};



class Planner
{
public:
  class PlanSet
  {
  public:
    virtual std::optional<const SchedulePatch> next_best() = 0;

    virtual ~PlanSet() = default;
  };

  virtual void set_current_schedule(
    std::shared_ptr<const AbstractScheduleState> state) = 0;

  virtual std::shared_ptr<PlanSet> plan(
    ConstraintTracker::RequestId request
  ) = 0;

  virtual std::shared_ptr<PlanSet> cancel(
    ConstraintTracker::RequestId req_id
  ) = 0;

  virtual ~Planner() = default;
};

//==============================================================================
// The min conflict planner is an example of a planner which services the
// reservation. It's aim is to minimize the number of conflicts and thus the
// amount of network bandwith used. This is essentially a greedy algortihm and
// does not guarantee maximum satisfaction.
class MinConflictPlanner : Planner
{
  class MinConflictPlanSet : Planner::PlanSet
  {
  public:
    ConstraintTracker::RequestId request;
    std::shared_ptr<const AbstractScheduleState> state;
    std::shared_ptr<ConstraintTracker> tracker;
    rmf::Duration sampling_interval;

    struct PriorityQueueEntry
    {
      std::shared_ptr<const AbstractScheduleState> ;
    }


    void dijkstra()
    {
      
    }

    std::optional<const SchedulePatch> next_best()
    {
      
    }
  };

  std::shared_ptr<const AbstractScheduleState> state;

  std::shared_ptr<ConstraintTracker> tracker;

  void set_current_schedule(
    std::shared_ptr<const AbstractScheduleState> _state) override
  {
    this->state = _state;
  }

  std::shared_ptr<PlanSet> plan(ConstraintTracker::RequestId request) override
  {
    auto res = std::make_shared<MinConflictPlanSet>();
    res->request = request;
    res->state = state;
    return res;
  }

  std::shared_ptr<PlanSet> cancel(ConstraintTracker::RequestId req_id)
  {
    return ;
  }
}

//==============================================================================
// Best-First-Search based planner. This has more fancy operations allowed.
}
}

#endif