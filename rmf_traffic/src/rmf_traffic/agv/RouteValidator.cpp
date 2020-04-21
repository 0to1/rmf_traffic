/*
 * Copyright (C) 2020 Open Source Robotics Foundation
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

#include <rmf_traffic/agv/RouteValidator.hpp>
#include <rmf_traffic/DetectConflict.hpp>

namespace rmf_traffic {
namespace agv {

//==============================================================================
class ScheduleRouteValidator::Implementation
{
public:

  const schedule::Viewer* viewer;
  schedule::ParticipantId participant;
  Profile profile;
  mutable rmf_traffic::schedule::Query query;

};

//==============================================================================
ScheduleRouteValidator::ScheduleRouteValidator(
  const schedule::Viewer& viewer,
  schedule::ParticipantId participant_id,
  Profile profile)
: _pimpl(rmf_utils::make_impl<Implementation>(
      Implementation{
        &viewer,
        participant_id,
        std::move(profile),
        rmf_traffic::schedule::query_all()
      }))
{
  _pimpl->query.spacetime().query_timespan({});
}

//==============================================================================
ScheduleRouteValidator& ScheduleRouteValidator::schedule_viewer(
  const schedule::Viewer& viewer)
{
  _pimpl->viewer = &viewer;
  return *this;
}

//==============================================================================
const schedule::Viewer& ScheduleRouteValidator::schedule_viewer() const
{
  return *_pimpl->viewer;
}

//==============================================================================
ScheduleRouteValidator& ScheduleRouteValidator::participant(
  const schedule::ParticipantId p)
{
  _pimpl->participant = p;
  return *this;
}

//==============================================================================
schedule::ParticipantId ScheduleRouteValidator::participant() const
{
  return _pimpl->participant;
}

//==============================================================================
rmf_utils::optional<schedule::ParticipantId>
ScheduleRouteValidator::find_conflict(const Route& route) const
{
  _pimpl->query.spacetime().timespan()->clear_maps();
  _pimpl->query.spacetime().timespan()->add_map(route.map());

  _pimpl->query.spacetime().timespan()->set_lower_time_bound(
    *route.trajectory().start_time());

  _pimpl->query.spacetime().timespan()->set_upper_time_bound(
    *route.trajectory().finish_time());

  const auto view = _pimpl->viewer->query(_pimpl->query);
  for (const auto& v : view)
  {
    if (v.participant == _pimpl->participant)
      continue;

    if (rmf_traffic::DetectConflict::between(
        _pimpl->profile,
        route.trajectory(),
        v.description.profile(),
        v.route.trajectory()))
      return v.participant;
  }

  return rmf_utils::nullopt;
}

//==============================================================================
std::unique_ptr<RouteValidator> ScheduleRouteValidator::clone() const
{
  return std::make_unique<ScheduleRouteValidator>(*this);
}

//==============================================================================
class NegotiatingRouteValidator::Generator::Implementation
{
public:
  struct Data
  {
    const schedule::Negotiation::Table* table;
    Profile profile;
  };

  std::shared_ptr<Data> data;
};

//==============================================================================
NegotiatingRouteValidator::Generator::Generator(
  const schedule::Negotiation::Table& table,
  Profile profile)
: _pimpl(rmf_utils::make_impl<Implementation>(
      Implementation{
        std::make_shared<Implementation::Data>(
         Implementation::Data{
           &table,
           std::move(profile)
         })
      }))
{
  // Do nothing
}

//==============================================================================
class NegotiatingRouteValidator::Implementation
{
public:

  std::shared_ptr<Generator::Implementation::Data> data;
  std::vector<schedule::Negotiation::Table::Rollout> rollouts;

  static NegotiatingRouteValidator make(
    std::shared_ptr<Generator::Implementation::Data> data,
    std::vector<schedule::Negotiation::Table::Rollout> rollouts)
  {
    NegotiatingRouteValidator output;
    output._pimpl = rmf_utils::make_impl<Implementation>(
          Implementation{
            std::move(data),
            std::move(rollouts)
          });

    return output;
  }
};

//==============================================================================
NegotiatingRouteValidator NegotiatingRouteValidator::Generator::begin() const
{
  std::vector<schedule::Negotiation::Table::Rollout> rollouts;
  for (const auto& r : _pimpl->data->table->rollouts())
    rollouts.push_back({r.first, 0});

  return NegotiatingRouteValidator::Implementation::make(
        _pimpl->data, std::move(rollouts));
}

//==============================================================================
rmf_utils::optional<schedule::ParticipantId>
NegotiatingRouteValidator::find_conflict(const Route& route) const
{
  // TODO(MXG): Consider if we can reduce the amount of heap allocation that's
  // needed here.
  auto query = schedule::make_query(
    {route.map()},
    route.trajectory().start_time(),
    route.trajectory().finish_time());

  const auto view = _pimpl->data->table->query(
        query.spacetime(), _pimpl->rollouts);

  for (const auto& v : view)
  {
    if (rmf_traffic::DetectConflict::between(
        _pimpl->data->profile,
        route.trajectory(),
        v.description.profile(),
        v.route.trajectory()))
    {
      return v.participant;
    }
  }

  return rmf_utils::nullopt;
}

//==============================================================================
std::unique_ptr<RouteValidator> NegotiatingRouteValidator::clone() const
{
  return std::make_unique<NegotiatingRouteValidator>(*this);
}

//==============================================================================
NegotiatingRouteValidator::NegotiatingRouteValidator()
{
  // Do nothing
}

} // namespace agv
} // namespace rmf_traffic
