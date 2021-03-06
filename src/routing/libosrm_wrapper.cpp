/*

This file is part of VROOM.

Copyright (c) 2015-2017, Julien Coupey.
All rights reserved (see LICENSE).

*/

#include "./libosrm_wrapper.h"

libosrm_wrapper::libosrm_wrapper(const std::string& osrm_profile)
  : osrm_wrapper(osrm_profile),
    _config(),
    _osrm(_config) {}

matrix<distance_t>
libosrm_wrapper::get_matrix(const std::vector<location_t>& locs) const {
  osrm::TableParameters params;
  for (auto const& location : locs) {
    assert(location.has_coordinates());
    params.coordinates.emplace_back(osrm::util::FloatLongitude({location.lon.get()}),
                                    osrm::util::FloatLatitude({location.lat.get()}));
  }

  osrm::json::Object result;
  osrm::Status status;
  try {
    status = _osrm.Table(params, result);
  }
  catch(const std::exception &e) {
    throw custom_exception(e.what());
  }

  if (status == osrm::Status::Error) {
    throw custom_exception("libOSRM: " +
                           result.values["code"].get<osrm::json::String>().value
                           + ": " +
                           result.values["message"].get<osrm::json::String>().value);
  }

  auto& table = result.values["durations"].get<osrm::json::Array>();

  // Expected matrix size.
  std::size_t m_size = locs.size();
  assert(table.values.size() == m_size);

  // Build matrix while checking for unfound routes to avoid
  // unexpected behavior (OSRM raises 'null').
  matrix<distance_t> m{m_size};

  std::vector<unsigned> nb_unfound_from_loc(m_size, 0);
  std::vector<unsigned> nb_unfound_to_loc(m_size, 0);

  std::string reason;
  for (std::size_t i = 0; i < m_size; ++i) {
    const auto& line = table.values.at(i).get<osrm::json::Array>();
    assert(line.values.size() == m_size);
    for (std::size_t j = 0; j < m_size; ++j) {
      const auto& el = line.values.at(j);
      if (el.is<osrm::json::Null>()) {
        // No route found between i and j. Just storing info as we
        // don't know yet which location is responsible between i
        // and j.
        ++nb_unfound_from_loc[i];
        ++nb_unfound_to_loc[j];
      }
      else {
        m[i][j] = round_to_distance(el.get<osrm::json::Number>().value);
      }
    }
  }

  check_unfound(locs, nb_unfound_from_loc, nb_unfound_to_loc);

  return m;
}

void libosrm_wrapper::add_route_geometry(route_t& rte) const {
  // Default options for routing.
  osrm::RouteParameters params(false, // steps
                               false, // alternatives
                               false, // annotations
                               osrm::RouteParameters::GeometriesType::Polyline,
                               osrm::RouteParameters::OverviewType::Full,
                               false // continue_straight
                               );

  // Ordering locations for the given steps.
  for (auto& step : rte.steps) {
    params.coordinates.emplace_back(osrm::util::FloatLongitude({step.location.lon.get()}),
                                    osrm::util::FloatLatitude({step.location.lat.get()}));
  }

  osrm::json::Object result;
  osrm::Status status;
  try {
    status = _osrm.Route(params, result);
  }
  catch(const std::exception &e) {
    throw custom_exception(e.what());
  }

  if (status == osrm::Status::Error) {
    throw custom_exception("libOSRM: " +
                           result.values["code"].get<osrm::json::String>().value
                           + ": " +
                           result.values["message"].get<osrm::json::String>().value);
  }

  auto& result_routes = result.values["routes"].get<osrm::json::Array>();
  auto& route = result_routes.values.at(0).get<osrm::json::Object>();

  rte.duration =
    round_to_distance(route.values["duration"].get<osrm::json::Number>().value);
  rte.distance =
    round_to_distance(route.values["distance"].get<osrm::json::Number>().value);
  rte.geometry =
    std::move(route.values["geometry"].get<osrm::json::String>().value);
}
