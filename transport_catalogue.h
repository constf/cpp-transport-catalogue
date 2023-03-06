#pragma once

#include <map>
#include <set>
#include <deque>
#include <string>
#include <vector>
#include <optional>
#include <unordered_map>
#include <functional>

#include "geo.h"
#include "domain.h"
#include "graph.h"


namespace transport_catalogue {

const double MET_MIN_RATIO = 1000.00 / 60.00;

struct RoutingSettings {
    int bus_wait_time;
    double bus_velocity;
};


struct StopsPointers {
public:
    const Stop* stop;
    const Stop* other;

    size_t operator()(const StopsPointers& st_pair) const{
        return hasher_(st_pair.stop) + 43*hasher_(st_pair.other);
    }

    bool operator()(const StopsPointers& lhs, const StopsPointers& rhs) const{
        return lhs.stop == rhs.stop && lhs.other == rhs.other;
    }

private:
    std::hash<const Stop*> hasher_;

};


class TransportCatalogue {
public:
    TransportCatalogue() = default;
    void AddStop(const std::string& name, const geo::Coordinates coords);
    void AddStop(const Stop& stop);
    std::pair<bool, const Stop&> FindStop(const std::string_view name) const;
    bool AddBus(const BusRoute& bus_route);
    const BusRoute& FindBus(std::string_view name);
    BusInfo GetBusInfo(std::string_view bus_name) const;
    const std::set<std::string_view>& GetBusesForStop(std::string_view stop) const;
    bool SetDistanceBetweenStops(std::string_view stop, std::string_view other_stop, int dist);
    int GetDistanceBetweenStops(std::string_view stop, std::string_view other_stop) const;
    const std::map<std::string_view, const BusRoute*> GetAllRoutesIndex() const;
    const std::map<std::string_view, const Stop*> GetAllStopsIndex() const;
    const std::unordered_map<std::string_view, const Stop*>& RawStopsIndex() const;
    size_t GetNumberOfStopsOnAllRoutes() const;
    const std::unordered_map<StopsPointers, int, StopsPointers, StopsPointers>& RawDistancesIndex() const;
    const std::unordered_map<std::string_view, std::set<std::string_view>>& GetStopAndBuses() const;

private:
    std::deque<Stop> stops_;
    std::unordered_map<std::string_view, const Stop*> stops_index_;

    std::deque<BusRoute> bus_routes_;
    std::unordered_map<std::string_view, const BusRoute*> routes_index_;

    std::unordered_map<std::string_view, std::set<std::string_view>> stop_and_buses_;
    std::unordered_map<StopsPointers, int, StopsPointers, StopsPointers> stops_distance_index_;
};


class TransportCatalogueGraph : public graph::DirectedWeightedGraph<double> {
public:
    struct StopOnRoute {
        size_t stop_number;
        std::string_view stop_name;
        std::string_view bus_name;

        explicit StopOnRoute(size_t num, std::string_view stop, std::string_view bus) : stop_number(num), stop_name(stop), bus_name(bus) {
        }
        StopOnRoute() = default;

        size_t operator()(const StopOnRoute& sor) const {
            return hasher_num_(stop_number) + 43 * hasher_(sor.stop_name) + 43 * 43 * hasher_(sor.bus_name);
        }
        bool operator()(const StopOnRoute& lhs, const StopOnRoute& rhs) const {
            return lhs.stop_name == rhs.stop_name && lhs.bus_name == rhs.bus_name && lhs.stop_number == rhs.stop_number;
        }
    private:
        std::hash<size_t> hasher_num_;
        std::hash<std::string_view> hasher_;
    };

public:
    TransportCatalogueGraph(const TransportCatalogue& tc, RoutingSettings rs);
    ~TransportCatalogueGraph() override = default;
    graph::VertexId GetStopVertexId(std::string_view stop_name) const;
    const StopOnRoute& GetStopById(graph::VertexId id) const;

private:
    const TransportCatalogue& tc_;
    RoutingSettings rs_;

    std::unordered_map<StopOnRoute, graph::VertexId , StopOnRoute, StopOnRoute> stop_to_vertex_;
    std::unordered_map<size_t , StopOnRoute> vertex_to_stop_;
    graph::VertexId vertex_id_count_ = 0;

    graph::VertexId RegisterStop(const StopOnRoute& stop);
    double CalculateTimeForDistance(int distance) const;
};


} // transport_catalogue namespace