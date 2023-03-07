#pragma once
#include "transport_catalogue.h"


struct RoutingSettings {
    int bus_wait_time;
    double bus_velocity;
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
    TransportCatalogueGraph(const transport_catalogue::TransportCatalogue& tc, RoutingSettings rs);
    ~TransportCatalogueGraph() = default;
    graph::VertexId GetStopVertexId(std::string_view stop_name, const std::string& dir) const;
    const StopOnRoute& GetStopById(graph::VertexId id) const;

private:
    const transport_catalogue::TransportCatalogue& tc_;
    RoutingSettings rs_;

    std::unordered_map<StopOnRoute, graph::VertexId , StopOnRoute, StopOnRoute> stop_to_vertex_;
    std::unordered_map<size_t , StopOnRoute> vertex_to_stop_;
    graph::VertexId vertex_id_count_ = 0;

    graph::VertexId RegisterStop(const StopOnRoute& stop);
    double CalculateTimeForDistance(int distance) const;
};