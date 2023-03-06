#include "transport_catalogue.h"
#include <numeric>
#include <utility>
#include <iostream>
#include <set>
#include <iomanip>

namespace transport_catalogue{


void TransportCatalogue::AddStop(const std::string& name, const geo::Coordinates coords) {
    const Stop stop {name, coords};

    AddStop(stop);
}

void TransportCatalogue::AddStop(const Stop& stop) {
    if (stops_index_.count(stop.stop_name) > 0) return;

    const Stop* ptr = &stops_.emplace_back(stop);

    std::string_view stop_name (ptr->stop_name); // string_view must point to permanent string, that will not disappear.
    stops_index_.emplace(stop_name, ptr);
}

std::pair<bool, const Stop&> TransportCatalogue::FindStop(const std::string_view name) const {
    const auto iter = stops_index_.find(name);
    if (iter == stops_index_.end()) return {false, EMPTY_STOP};

    return {true, *iter->second};
}

bool TransportCatalogue::AddBus(const BusRoute &bus_route) {
    const auto iter = routes_index_.find(bus_route.bus_name);
    if (iter != routes_index_.end()) return false;

    const BusRoute* ptr = &bus_routes_.emplace_back(bus_route);

    std::string_view bus_name (ptr->bus_name);
    routes_index_.emplace(bus_name, ptr);

    for (const Stop* stop : ptr->route_stops) {
        stop_and_buses_[stop->stop_name].insert(bus_name);
    }

    return true;
}

const BusRoute& TransportCatalogue::FindBus(std::string_view name) {
    const auto iter = routes_index_.find(name);
    if (iter == routes_index_.end()) return EMPTY_BUS_ROUTE;

    return *iter->second;
}

BusInfo TransportCatalogue::GetBusInfo(std::string_view bus_name) const {
    BusInfo result;
    result.type = RouteType::NOT_SET;

    auto iter = routes_index_.find(bus_name);
    if (iter == routes_index_.end()) return result;

    const BusRoute& route = *iter->second;

    std::set<const Stop*> unique_stops(route.route_stops.begin(), route.route_stops.end());
    result.unique_stops = unique_stops.size();

    double length_geo = 0.0;
    size_t length_meters = 0;
    for(auto first = route.route_stops.begin(); first != route.route_stops.end(); ++first) {
        auto second = std::next(first);
        if (second == route.route_stops.end()) break;

        // compute the shortest distance by the straight line
        length_geo += ComputeDistance((**first).coordinates, (**second).coordinates);

        // compute the road length
        length_meters += GetDistanceBetweenStops((**first).stop_name, (**second).stop_name);
        // if it is a way and back route, add the back distance, which may be different from direct distance
        if (route.type == RouteType::RETURN_ROUTE) {
            length_meters += GetDistanceBetweenStops((**second).stop_name, (**first).stop_name);
        }
    }

    result.stops_number = route.route_stops.size();
    if (route.type == RouteType::RETURN_ROUTE){
        result.stops_number *= 2;
        result.stops_number -= 1;
        length_geo *= 2;
    }
    result.route_length = length_meters;
    result.curvature = static_cast<double>(length_meters) / length_geo;
    result.bus_name = route.bus_name;
    result.type = route.type;

    return result;
}

const std::set<std::string_view>& TransportCatalogue::GetBusesForStop(std::string_view stop) const {
    const auto iter = stop_and_buses_.find(stop);

    if (iter == stop_and_buses_.end()) {
        return EMPTY_BUS_ROUTE_SET;
    }

    return iter->second;
}

bool TransportCatalogue::SetDistanceBetweenStops(std::string_view stop, std::string_view other_stop, int dist) {
    auto iter_stop = stops_index_.find(stop);
    auto iter_other = stops_index_.find(other_stop);
    if (iter_stop == stops_index_.end() || iter_other == stops_index_.end()) return false; // one of stops is not present in the catalogue

    // insert direct pair without any check. it is either first insert or value substitute.
    StopsPointers direct {};
    direct.stop = iter_stop->second;
    direct.other = iter_other->second;
    stops_distance_index_[direct] = dist;

    StopsPointers reverse {};
    reverse.stop = direct.other;
    reverse.other = direct.stop;
    // for the reverse pair, let's check if it already exists
    auto iter_rev = stops_distance_index_.find(reverse);
    if (iter_rev == stops_distance_index_.end()) {
        // insert the reverse pair if it does not exist
        // if exists, it means that the distance might be different for the direct and reverse
        stops_distance_index_[reverse] = dist;
    }

    return true;
}

int TransportCatalogue::GetDistanceBetweenStops(std::string_view stop, std::string_view other_stop) const {
    const auto iter_stop = stops_index_.find(stop);
    const auto iter_other = stops_index_.find(other_stop);
    if (iter_stop == stops_index_.end() || iter_other == stops_index_.end()) return -1;

    StopsPointers direct {};
    direct.stop = iter_stop->second;
    direct.other = iter_other->second;

    auto iter_dist = stops_distance_index_.find(direct);
    if (iter_dist == stops_distance_index_.end()) return -1;

    return iter_dist->second;
}

const std::map<std::string_view, const BusRoute*> TransportCatalogue::GetAllRoutesIndex() const {
    std::map<std::string_view, const BusRoute*>  result(routes_index_.begin(), routes_index_.end());
    return result;
}

const std::map<std::string_view, const Stop *> TransportCatalogue::GetAllStopsIndex() const {
    std::map<std::string_view, const Stop*> result(stops_index_.begin(), stops_index_.end());
    return result;
}

const std::unordered_map<std::string_view, const Stop *>&
TransportCatalogue::RawStopsIndex() const {
    return stops_index_;
}

const std::unordered_map<StopsPointers, int, StopsPointers, StopsPointers>&
TransportCatalogue::RawDistancesIndex() const {
    return stops_distance_index_;
}

const std::unordered_map<std::string_view, std::set<std::string_view>> &
TransportCatalogue::GetStopAndBuses() const {
    return stop_and_buses_;
}

    size_t TransportCatalogue::GetNumberOfStopsOnAllRoutes() const {
        size_t result = 0;

        for (const auto& [bus, route] : routes_index_) {
            result += route->route_stops.size();
            if (route->type == RouteType::CIRCLE_ROUTE) {
                --result;
            }
        }

        return result;
    }


    TransportCatalogueGraph::TransportCatalogueGraph(const TransportCatalogue& tc, RoutingSettings rs):
    graph::DirectedWeightedGraph<double>(tc.RawStopsIndex().size() + tc.GetNumberOfStopsOnAllRoutes()), tc_(tc), rs_(rs) {
    // Get all bus routes
    const auto& routes_index = tc_.GetAllRoutesIndex();

    // iterate for all routes
    for (const auto& [bus, bus_route] : routes_index) {
        // iterate all stops in a route
        size_t stop_count = 0;
        for (auto first = bus_route->route_stops.begin(); first != bus_route->route_stops.end(); ++first) {
            // route may be Circle or Return
            auto second = first + 1;

            // from vertices, stop and stop on the bus route
            StopOnRoute stop_from {0, (*first)->stop_name, {}}; // stop name, for any bus route
            StopOnRoute stop_bus_from {stop_count++, (*first)->stop_name, bus}; // stop name for current bus route

            if (second == bus_route->route_stops.end()) { // last stop, make connection from stop to stop on the bus route
                if (bus_route->type == RouteType::RETURN_ROUTE) {
                    AddEdge({RegisterStop(stop_from), RegisterStop(stop_bus_from), static_cast<double>(rs_.bus_wait_time)});
                }
                break;
            }

            // to vertices, stop and stop on the bus route
            StopOnRoute stop_to {0, (*second)->stop_name, {}}; // stop name, for any bus route
            StopOnRoute stop_bus_to {stop_count, (*second)->stop_name, bus}; // stop name for current bus route

            // Register Edges
            int distance = tc_.GetDistanceBetweenStops((*first)->stop_name, (*second)->stop_name);
            double time_between_stops = CalculateTimeForDistance(distance); // travel time between 2 stops
            AddEdge({RegisterStop(stop_from), RegisterStop(stop_bus_from), static_cast<double>(rs_.bus_wait_time)});
            AddEdge({RegisterStop(stop_bus_from), RegisterStop(stop_to), time_between_stops});
            // if it is a circle route and it is not the last stop
            if (bus_route->type == RouteType::RETURN_ROUTE || std::next(second) != bus_route->route_stops.end()) {
                // AddEdge({RegisterStop(stop_to), RegisterStop(stop_bus_to), static_cast<double>(rs_.bus_wait_time)});
                AddEdge({RegisterStop(stop_bus_from), RegisterStop(stop_bus_to), time_between_stops});
            }

            if (bus_route->type == RouteType::CIRCLE_ROUTE) continue;

            // Register opposite direction for return route
            distance = tc_.GetDistanceBetweenStops((*second)->stop_name, (*first)->stop_name);
            time_between_stops = CalculateTimeForDistance(distance); // travel time between 2 stops
            AddEdge({RegisterStop(stop_bus_to), RegisterStop(stop_from), time_between_stops});
            AddEdge({RegisterStop(stop_bus_to), RegisterStop(stop_bus_from), time_between_stops});
        }
    }
}


graph::VertexId TransportCatalogueGraph::RegisterStop(const StopOnRoute& stop) {
    auto iter = stop_to_vertex_.find(stop);
    if (iter != stop_to_vertex_.end()) {
        return iter->second; // return the stop vertex number, it already exists in the map
    }

    auto result = vertex_id_count_;

    stop_to_vertex_[stop] = vertex_id_count_;
    vertex_to_stop_[vertex_id_count_] = stop;
    ++vertex_id_count_;

    return result;
}


double TransportCatalogueGraph::CalculateTimeForDistance(int distance) const {
    return static_cast<double>(distance) / (rs_.bus_velocity * MET_MIN_RATIO);
}


graph::VertexId TransportCatalogueGraph::GetStopVertexId(std::string_view stop_name) const {
    StopOnRoute stop{0,stop_name, {}};
    auto iter = stop_to_vertex_.find(stop);

    if (iter != stop_to_vertex_.end()) {
        return iter->second; // return the stop vertex number, if it exists in the map
    }

    throw std::logic_error("Error, no stop name: " + std::string (stop_name));
}

    const TransportCatalogueGraph::StopOnRoute& TransportCatalogueGraph::GetStopById(graph::VertexId id) const {
        return vertex_to_stop_.at(id);
    }

} // transport_catalogue namespace
