#pragma once

#include "graph.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iterator>
#include <optional>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <vector>
#include "transport_catalogue.pb.h"

namespace graph {

    template <typename Weight>
    class Router {
    private:
        using Graph = DirectedWeightedGraph<Weight>;

    public:
        explicit Router(const Graph& graph);
        explicit Router(const Graph& graph, const tc_serialize::TransportCatalogue& tc_pbuf);

        bool SaveTo(tc_serialize::TransportCatalogue& tc_out) const;
        bool RestoreFrom(const tc_serialize::TransportCatalogue& tc_in);

        struct RouteInfo {
            Weight weight;
            std::vector<EdgeId> edges;
        };

        std::optional<RouteInfo> BuildRoute(VertexId from, VertexId to) const;

        struct RouteInternalData {
            Weight weight;
            std::optional<EdgeId> prev_edge;
        };
        using RoutesInternalData = std::vector<std::vector<std::optional<RouteInternalData>>>;

    private:
        tc_serialize::RouteIntDataPB RouteIntDataToSerialize(const std::optional<Router<Weight>::RouteInternalData> &data) const;
        std::optional<RouteInternalData> RouteIntDataToDomain(const tc_serialize::RouteIntDataPB& data) const;

        void InitializeRoutesInternalData(const Graph& graph) {
            const size_t vertex_count = graph.GetVertexCount();
            for (VertexId vertex = 0; vertex < vertex_count; ++vertex) {
                routes_internal_data_[vertex][vertex] = RouteInternalData{ZERO_WEIGHT, std::nullopt};
                for (const EdgeId edge_id : graph.GetIncidentEdges(vertex)) {
                    const auto& edge = graph.GetEdge(edge_id);
                    if (edge.weight < ZERO_WEIGHT) {
                        throw std::domain_error("Edges' weights should be non-negative");
                    }
                    auto& route_internal_data = routes_internal_data_[vertex][edge.to];
                    if (!route_internal_data || route_internal_data->weight > edge.weight) {
                        route_internal_data = RouteInternalData{edge.weight, edge_id};
                    }
                }
            }
        }

        void RelaxRoute(VertexId vertex_from, VertexId vertex_to, const RouteInternalData& route_from,
                        const RouteInternalData& route_to) {
            auto& route_relaxing = routes_internal_data_[vertex_from][vertex_to];
            const Weight candidate_weight = route_from.weight + route_to.weight;
            if (!route_relaxing || candidate_weight < route_relaxing->weight) {
                route_relaxing = {candidate_weight,
                                  route_to.prev_edge ? route_to.prev_edge : route_from.prev_edge};
            }
        }

        void RelaxRoutesInternalDataThroughVertex(size_t vertex_count, VertexId vertex_through) {
            for (VertexId vertex_from = 0; vertex_from < vertex_count; ++vertex_from) {
                if (const auto& route_from = routes_internal_data_[vertex_from][vertex_through]) {
                    for (VertexId vertex_to = 0; vertex_to < vertex_count; ++vertex_to) {
                        if (const auto& route_to = routes_internal_data_[vertex_through][vertex_to]) {
                            RelaxRoute(vertex_from, vertex_to, *route_from, *route_to);
                        }
                    }
                }
            }
        }

        static constexpr Weight ZERO_WEIGHT{};
        const Graph& graph_;
        RoutesInternalData routes_internal_data_;
    };

    template<typename Weight>
    std::optional<typename Router<Weight>::RouteInternalData>
    Router<Weight>::RouteIntDataToDomain(const tc_serialize::RouteIntDataPB &data) const {
        if (data.data_empty()) {
            return {std::nullopt};
        }

        std::optional<typename Router<Weight>::RouteInternalData> result{Router<Weight>::RouteInternalData {}};
        if (!data.prev_edge_empty()) {
            result->prev_edge = data.prev_edge();
        }
        result->weight = data.weight();

        return result;
    }


    template<typename Weight>
    tc_serialize::RouteIntDataPB
    Router<Weight>::RouteIntDataToSerialize(const std::optional<Router<Weight>::RouteInternalData> &data) const {
        tc_serialize::RouteIntDataPB result;
        if (!data) {
            result.set_data_empty(true);
            return result;
        }

        result.set_data_empty(false);
        if (data->prev_edge) {
            result.set_prev_edge_empty(false);
            result.set_prev_edge(data->prev_edge.value());
        } else {
            result.set_prev_edge_empty(true);
        }
        result.set_weight(data->weight);

        return result;
    }



    template<typename Weight>
    bool Router<Weight>::RestoreFrom(const tc_serialize::TransportCatalogue &tc_in) {
        routes_internal_data_.reserve(tc_in.router_routes_int_data().routes_list_size());

        for (int i = 0; i < tc_in.router_routes_int_data().routes_list_size(); ++i) {
            const tc_serialize::VertexCountListPB& small_list = tc_in.router_routes_int_data().routes_list(i);
            std::vector<std::optional<RouteInternalData>> out_list(small_list.vertex_list_size());
            for (int j = 0; j < small_list.vertex_list_size(); ++j) {
                const tc_serialize::RouteIntDataPB& data = small_list.vertex_list(j);
                out_list[j] = RouteIntDataToDomain(data);
            }
            routes_internal_data_.emplace_back(std::move(out_list));
        }

        return true;
    }

    template<typename Weight>
    bool Router<Weight>::SaveTo(tc_serialize::TransportCatalogue &tc_out) const {
        tc_serialize::RoutesInternalDataListsPB big_list;

        for (const auto& vertex_list : routes_internal_data_) {
            tc_serialize::VertexCountListPB int_data_list;
            for (const auto& data : vertex_list) {
                *int_data_list.add_vertex_list() = RouteIntDataToSerialize(data);
            }
            *big_list.add_routes_list() = std::move(int_data_list);
        }

        *tc_out.mutable_router_routes_int_data() = std::move(big_list);

        return true;
    }

    template<typename Weight>
    Router<Weight>::Router(const Router::Graph &graph, const tc_serialize::TransportCatalogue &tc_pbuf)
            : graph_(graph) {
        RestoreFrom(tc_pbuf);
    }

    template <typename Weight>
    Router<Weight>::Router(const Graph& graph)
            : graph_(graph)
            , routes_internal_data_(graph.GetVertexCount(),
                                    std::vector<std::optional<RouteInternalData>>(graph.GetVertexCount()))
    {
        InitializeRoutesInternalData(graph);

        const size_t vertex_count = graph.GetVertexCount();
        for (VertexId vertex_through = 0; vertex_through < vertex_count; ++vertex_through) {
            RelaxRoutesInternalDataThroughVertex(vertex_count, vertex_through);
        }
    }

    template <typename Weight>
    std::optional<typename Router<Weight>::RouteInfo> Router<Weight>::BuildRoute(VertexId from,
                                                                                 VertexId to) const {
        const auto& route_internal_data = routes_internal_data_.at(from).at(to);
        if (!route_internal_data) {
            return std::nullopt;
        }
        const Weight weight = route_internal_data->weight;
        std::vector<EdgeId> edges;
        for (std::optional<EdgeId> edge_id = route_internal_data->prev_edge;
             edge_id;
             edge_id = routes_internal_data_[from][graph_.GetEdge(*edge_id).from]->prev_edge)
        {
            edges.push_back(*edge_id);
        }
        std::reverse(edges.begin(), edges.end());

        return RouteInfo{weight, std::move(edges)};
    }
}  // namespace graph