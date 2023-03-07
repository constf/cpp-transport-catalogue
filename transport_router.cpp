#include "transport_router.h"


TransportCatalogueGraph::TransportCatalogueGraph(const transport_catalogue::TransportCatalogue& tc, RoutingSettings rs):
        graph::DirectedWeightedGraph<double>(tc.RawStopsIndex().size() + tc.GetNumberOfStopsOnAllRoutes()), tc_(tc), rs_(rs) {
    // Get all bus routes
    const auto& routes_index = tc_.GetAllRoutesIndex();
    const auto& just_all_stops = tc_.RawStopsIndex();

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
                if (bus_route->type == transport_catalogue::RouteType::RETURN_ROUTE) {
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
            if (bus_route->type == transport_catalogue::RouteType::RETURN_ROUTE || std::next(second) != bus_route->route_stops.end()) {
                // AddEdge({RegisterStop(stop_to), RegisterStop(stop_bus_to), static_cast<double>(rs_.bus_wait_time)});
                AddEdge({RegisterStop(stop_bus_from), RegisterStop(stop_bus_to), time_between_stops});
            }

            if (bus_route->type == transport_catalogue::RouteType::CIRCLE_ROUTE) continue;

            // Register opposite direction for return route
            distance = tc_.GetDistanceBetweenStops((*second)->stop_name, (*first)->stop_name);
            time_between_stops = CalculateTimeForDistance(distance); // travel time between 2 stops
            AddEdge({RegisterStop(stop_bus_to), RegisterStop(stop_from), time_between_stops});
            AddEdge({RegisterStop(stop_bus_to), RegisterStop(stop_bus_from), time_between_stops});
        }
    }

    // now, check all stops to find those that do not belong to any routes
    for (const auto& [stop_name, stop_ptr] : just_all_stops) {
        StopOnRoute stop {0, stop_name, {}};
        RegisterStop(stop);
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
    return static_cast<double>(distance) / (rs_.bus_velocity * transport_catalogue::MET_MIN_RATIO);
}

graph::VertexId TransportCatalogueGraph::GetStopVertexId(std::string_view stop_name, const std::string& dir) const {
    StopOnRoute stop{0,stop_name, {}};
    auto iter = stop_to_vertex_.find(stop);

    if (iter != stop_to_vertex_.end()) {
        return iter->second; // return the stop vertex number, if it exists in the map
    }

    throw std::logic_error("Error, no stop name: " + std::string (stop_name) + " direction: " + dir);
}

const TransportCatalogueGraph::StopOnRoute& TransportCatalogueGraph::GetStopById(graph::VertexId id) const {
    return vertex_to_stop_.at(id);
}