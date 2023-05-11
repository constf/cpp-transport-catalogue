#include "transport_catalogue.h"
#include <numeric>
#include <utility>
#include <iostream>
#include <set>
#include <iomanip>

namespace transport_catalogue{


void TransportCatalogue::AddStop(const std::string& name, const geo::Coordinates coords) {
    const Stop stop {0, name, coords};

    AddStop(stop);
}

void TransportCatalogue::AddStop(const Stop& stop) {
    if (stops_index_.count(stop.stop_name) > 0) return;

    Stop* ptr = &stops_.emplace_back(stop);

    if (ptr->id == 0){ // if we created the stop from Raw data in JSON
        ptr->id = ++stop_id_counter_;
    } else {
        stop_id_counter_ = ptr->id;
    }
    std::string_view stop_name(ptr->stop_name); // string_view must point to permanent string, that will not disappear.
    stops_index_.emplace(stop_name, ptr);
    stop_id_name_index_.emplace(ptr->id, ptr);
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

const BusRoute& TransportCatalogue::FindBus(std::string_view name) const {
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

void TransportCatalogue::SaveTo(tc_serialize::TransportCatalogue& t_cat) const {
    // Preparing  Stops
    tc_serialize::StopsList st_list;
    for (const auto [_, stop_ptr] : stop_id_name_index_ ) {
        *st_list.add_all_stops() = std::move(StopToSerialize(*stop_ptr));
    }
    *t_cat.mutable_stops() = st_list;

    // Preparing stop distances
    tc_serialize::StopDistanceIndex stop_distances;
    for (const auto [stop_ptrs, distance] : stops_distance_index_) {
        *stop_distances.add_all_stops_distance_index() = std::move(DistanceToSerialize(stop_ptrs.stop->id, stop_ptrs.other->id, distance));
    }
    *t_cat.mutable_index() = stop_distances;

    // Preparing bus routes
    tc_serialize::AllRoutesList routes_list;
    for (const BusRoute& route : bus_routes_) {
        tc_serialize::BusRoute br_out;
        br_out.set_bus_name(route.bus_name);
        int32_t rt = 0;
        if (route.type == RouteType::CIRCLE_ROUTE) {
            rt = 1;
        } else if (route.type == RouteType::RETURN_ROUTE) {
            rt = 2;
        } else {
            rt = 0;
        }
        br_out.set_route_type(rt);
        for (auto stop_ptr : route.route_stops) {
            br_out.add_stop_ids(stop_ptr->id);
        }
        *routes_list.add_routes_list() = std::move(br_out);
    }
    *t_cat.mutable_routes() = routes_list;
}


bool TransportCatalogue::RestoreFrom(tc_serialize::TransportCatalogue& t_cat) {
    // Restore stops
    tc_serialize::StopsList st_list = t_cat.stops();
    for (int i = 0; i < st_list.all_stops_size(); ++i) {
        AddStop(StopToDomain(st_list.all_stops(i)));
    }

    // Restores distances between stops
    tc_serialize::StopDistanceIndex stops_distances = t_cat.index();
    for (int i = 0; i < stops_distances.all_stops_distance_index_size(); ++i) {
        const tc_serialize::DistanceBetweenStops& dist = stops_distances.all_stops_distance_index(i);
        std::string_view from = GetStopNameById(dist.from_id());
        std::string_view to = GetStopNameById(dist.to_id());
        SetDistanceBetweenStops(from, to, dist.distance());
    }

    // Restore bus routes
    tc_serialize::AllRoutesList all_routes = t_cat.routes();
    for (int i = 0; i < all_routes.routes_list_size(); ++i) {
        const tc_serialize::BusRoute& route_in = all_routes.routes_list(i);
        BusRoute bus_out;
        bus_out.bus_name = route_in.bus_name();
        if (route_in.route_type() == 1) {
            bus_out.type = RouteType::CIRCLE_ROUTE;
        } else if (route_in.route_type() == 2) {
            bus_out.type = RouteType::RETURN_ROUTE;
        } else {
            bus_out.type = RouteType::NOT_SET;
        }
        bus_out.route_stops.reserve(route_in.stop_ids_size());
        for (int j = 0; j < route_in.stop_ids_size(); ++j) {
            const std::string_view stop_name = GetStopNameById(route_in.stop_ids(j));
            const auto [result, stop_const_ref] = FindStop(stop_name);
            if (!result) {
                return false;
            }
            bus_out.route_stops.push_back(&stop_const_ref);
        }
        AddBus(bus_out);
    }

    return true;
}



uint32_t TransportCatalogue::GetStopId(const std::string_view stop_name) const {
    auto iter = stops_index_.find(stop_name);
    if (iter == stops_index_.end()) {
        return 0;
    }

    return iter->second->id;
}

const std::string_view TransportCatalogue::GetStopNameById(uint32_t stop_id) const {
    auto iter = stop_id_name_index_.find(stop_id);
    if (iter == stop_id_name_index_.end()) {
        return {};
    }

    return {iter->second->stop_name};
}

} // transport_catalogue namespace
