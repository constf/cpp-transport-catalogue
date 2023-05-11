#include "transport_router.h"


TransportCatalogueRouterGraph::TransportCatalogueRouterGraph(const transport_catalogue::TransportCatalogue& tc, RoutingSettings rs):
        graph::DirectedWeightedGraph<double>(tc.RawStopsIndex().size()), tc_(tc), rs_(rs) {

    const auto& routes_index = tc_.GetAllRoutesIndex(); // Get all bus routes for all stops on routes

    // Register all stops that we have in catalogue as vertices
    for (const auto& [stop_name, stop_ptr] : tc_.RawStopsIndex()) {
        StopOnRoute stop {0, stop_name, {}};
        RegisterStop(stop);
    }

    // iterate for all routes
    for (const auto& [_, bus_route] : routes_index) {
        if (bus_route->type == transport_catalogue::RouteType::RETURN_ROUTE) {
            FillWithReturnRouteStops(bus_route);
        } else {
            FillWithCircleRouteStops(bus_route);
        }
    }

    router_ptr_ = std::make_unique<graph::Router<double>>(*this);
}

void TransportCatalogueRouterGraph::FillWithReturnRouteStops(const transport_catalogue::BusRoute *bus_route) {
    // iterate all stops in a route
    for (auto start = bus_route->route_stops.begin(); start != bus_route->route_stops.end(); ++start) {
        size_t stop_distance = 1; // count between stops
        int accumulated_distance_direct = 0;
        int accumulated_distance_reverse = 0;

        const auto wait_time_at_stop = static_cast<double>(rs_.bus_wait_time);

        auto from_id = GetStopVertexId((*start)->stop_name);
        for (auto first = start, second = start + 1; second != bus_route->route_stops.end(); ++first, ++second) {
            auto to_id = GetStopVertexId((*second)->stop_name);

            TwoStopsLink direct_link(bus_route->bus_name, from_id, to_id, stop_distance);
            const int direct_distance = tc_.GetDistanceBetweenStops((*first)->stop_name, (*second)->stop_name);
            accumulated_distance_direct += direct_distance;
            const double direct_link_time = wait_time_at_stop + CalculateTimeForDistance(accumulated_distance_direct); // travel time between 2 stops
            const auto direct_edge_id = AddEdge({from_id, to_id, direct_link_time});
            StoreLink(direct_link, direct_edge_id);

            // make reverse link and register Edge
            TwoStopsLink reverse_link(bus_route->bus_name, to_id, from_id, stop_distance);
            const int reverse_distance = tc_.GetDistanceBetweenStops((*second)->stop_name, (*first)->stop_name);
            accumulated_distance_reverse += reverse_distance;
            const double reverse_link_time = wait_time_at_stop + CalculateTimeForDistance(accumulated_distance_reverse); // travel time between 2 stops
            const auto reverse_edge_id = AddEdge({to_id, from_id, reverse_link_time});
            StoreLink(reverse_link, reverse_edge_id);

            ++stop_distance;

            edge_count_ = reverse_edge_id;
        }
    }
}

void TransportCatalogueRouterGraph::FillWithCircleRouteStops(const transport_catalogue::BusRoute *bus_route) {
    // iterate all stops in a route
    for (auto start = bus_route->route_stops.begin(); start != bus_route->route_stops.end(); ++start) {
        size_t stop_distance = 1; // count between stops
        int accumulated_distance_direct = 0;

        const auto wait_time_at_stop = static_cast<double>(rs_.bus_wait_time);

        auto from_id = GetStopVertexId((*start)->stop_name);
        for (auto first = start, second = start + 1; second != bus_route->route_stops.end(); ++first, ++second) {
            auto to_id = GetStopVertexId((*second)->stop_name);

            // make direct link and register Edge
            TwoStopsLink direct_link(bus_route->bus_name, from_id, to_id, stop_distance);
            const int direct_distance = tc_.GetDistanceBetweenStops((*first)->stop_name, (*second)->stop_name);
            accumulated_distance_direct += direct_distance;
            const double direct_link_time = wait_time_at_stop + CalculateTimeForDistance(accumulated_distance_direct); // travel time between 2 stops
            const auto direct_edge_id = AddEdge({from_id, to_id, direct_link_time});
            StoreLink(direct_link, direct_edge_id);

            ++stop_distance;

            edge_count_ = direct_edge_id;
        }
    }
}



graph::VertexId TransportCatalogueRouterGraph::RegisterStop(const StopOnRoute& stop) {
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

std::optional<graph::EdgeId> TransportCatalogueRouterGraph::CheckLink(const TwoStopsLink &link) const {
    auto iter = stoplink_to_edge_.find(link);

    if (iter != stoplink_to_edge_.end()) {
        return iter->second; // return the stop vertex number, it already exists in the map
    }

    return {};
}

graph::EdgeId TransportCatalogueRouterGraph::StoreLink(const TwoStopsLink &link, graph::EdgeId edge) {
    auto iter  = edge_to_stoplink_.find(edge);
    if (iter != edge_to_stoplink_.end()) {
        return iter->first;
    }

    stoplink_to_edge_[link] = edge;
    edge_to_stoplink_[edge] = link;

    return edge;
}

double TransportCatalogueRouterGraph::CalculateTimeForDistance(int distance) const {
    return static_cast<double>(distance) / (rs_.bus_velocity * transport_catalogue::MET_MIN_RATIO);
}

graph::VertexId TransportCatalogueRouterGraph::GetStopVertexId(std::string_view stop_name) const {
    StopOnRoute stop{0, stop_name, {}};
    auto iter = stop_to_vertex_.find(stop);

    if (iter != stop_to_vertex_.end()) {
        return iter->second; // return the stop vertex number, if it exists in the map
    }

    throw std::logic_error("Error, no stop name: " + std::string (stop_name));
}

const TransportCatalogueRouterGraph::StopOnRoute& TransportCatalogueRouterGraph::GetStopById(graph::VertexId id) const {
    return vertex_to_stop_.at(id);
}

double TransportCatalogueRouterGraph::GetBusWaitingTime() const {
    return static_cast<double>(rs_.bus_wait_time);
}

const TwoStopsLink& TransportCatalogueRouterGraph::GetLinkById(graph::EdgeId id) const {
    auto iter = edge_to_stoplink_.find(id);

    if (iter != edge_to_stoplink_.end()) {
        return iter->second;
    }

    throw std::logic_error("Error fetching the TwoStopsLink, no Edge id: " + std::to_string(id));
}

std::optional<graph::Router<double>::RouteInfo> TransportCatalogueRouterGraph::BuildRoute(std::string_view from, std::string_view to) const {
    if (!router_ptr_) return {};

    graph::VertexId from_id = GetStopVertexId(from);
    graph::VertexId to_id = GetStopVertexId(to);

    return router_ptr_->BuildRoute(from_id, to_id);
}

TransportCatalogueRouterGraph::TransportCatalogueRouterGraph(const transport_catalogue::TransportCatalogue &tc, RoutingSettings rs,
    const tc_serialize::TransportCatalogue &tc_pbuf) : tc_(tc), rs_(rs) {
    RestoreFrom(tc_pbuf);

    router_ptr_ = std::make_unique<graph::Router<double>>(*this, tc_pbuf);
}

bool TransportCatalogueRouterGraph::SaveTo(tc_serialize::TransportCatalogue& tc_out) const {
    tc_serialize::TCGraphRouter out;

    // Saving fields of TransportCatalogueRouterGraph
    // stop_to_vertex_ & vertex_to_stop_ saving - they have same pair
    for (const auto& [stop, vertex] : stop_to_vertex_) {
        *out.add_tc_router_stops_() = std::move(StopOnRouteToSerialize(stop, vertex));
    }
    // stoplink_to_edge_ & edge_to_stoplink_ saving - they have same pair
    for (const auto& [stoplink, edge] : stoplink_to_edge_) {
        *out.add_tc_router_links() = std::move(TwoStopsLinkToSerialize(stoplink, edge));
    }
    out.set_vertex_id_count(vertex_id_count_);
    out.set_edge_count(edge_count_);

    // Saving fields of graph::DirectedWeightedGraph
    // edges_
    tc_serialize::EdgeVectorPB edges_list;
    for (const auto& edge : edges_) {
        *edges_list.add_edges() = std::move(EdgeToSerialize(edge));
    }
    *out.mutable_graph_edges() = std::move(edges_list);
    // incidence_lists_
    tc_serialize::IncidenceListPB big_list;
    for (const auto& list : incidence_lists_) {
        *big_list.add_lists() = std::move(IncListToSerialize(list));
    }
    *out.mutable_graph_incidence_list() = std::move(big_list);

    *tc_out.mutable_tc_graph_router() = std::move(out);

    router_ptr_->SaveTo(tc_out);

    return true;
}

bool TransportCatalogueRouterGraph::RestoreFrom(const tc_serialize::TransportCatalogue &tc_in) {
    // Restoring fields of TransportCatalogueRouterGraph
    // stop_to_vertex_ & vertex_to_stop_ saving - they have same pair
    for (int i = 0; i < tc_in.tc_graph_router().tc_router_stops__size(); ++i) {
        StopOnRoute stop = StopOnRouteToDomain(tc_in.tc_graph_router().tc_router_stops_(i));
        graph::VertexId vertex_id = tc_in.tc_graph_router().tc_router_stops_(i).vertex_id();
        stop_to_vertex_[stop] = vertex_id;
        vertex_to_stop_[vertex_id] = stop;
    }
    // stoplink_to_edge_ & edge_to_stoplink_ saving - they have same pair
    for (int i = 0; i < tc_in.tc_graph_router().tc_router_links_size(); ++i) {
        TwoStopsLink link = TwoStopsLinkToDomain(tc_in.tc_graph_router().tc_router_links(i));
        graph::EdgeId edge_id = tc_in.tc_graph_router().tc_router_links(i).edge_id();
        stoplink_to_edge_[link] = edge_id;
        edge_to_stoplink_[edge_id] = link;
    }
    vertex_id_count_ = tc_in.tc_graph_router().vertex_id_count();
    edge_count_ = tc_in.tc_graph_router().edge_count();

    // Restoring fields of graph::DirectedWeightedGraph
    // edges_
    edges_.reserve(tc_in.tc_graph_router().graph_edges().edges_size());
    for (int i = 0; i < tc_in.tc_graph_router().graph_edges().edges_size(); ++i) {
        graph::Edge<double> edge = EdgeToDomain(tc_in.tc_graph_router().graph_edges().edges(i));
        edges_.emplace_back(edge);
    }
    // incidence_lists_
    incidence_lists_.reserve(tc_in.tc_graph_router().graph_incidence_list().lists_size());
    for (int i = 0; i < tc_in.tc_graph_router().graph_incidence_list().lists_size(); ++i) {
        auto list = std::move(IncListToDomain(tc_in.tc_graph_router().graph_incidence_list().lists(i)));
        incidence_lists_.emplace_back(std::move(list));
    }

    return true;
}



tc_serialize::StopOnRoutePB
TransportCatalogueRouterGraph::StopOnRouteToSerialize(const TransportCatalogueRouterGraph::StopOnRoute &stop, graph::VertexId vertexId) const {
    tc_serialize::StopOnRoutePB result;

    result.set_bus_name(std::string {stop.bus_name});
    result.set_stop_number(stop.stop_number);
    result.set_stop_id(tc_.GetStopId(stop.stop_name));

    result.set_vertex_id(vertexId);

    return std::move(result);
}

TransportCatalogueRouterGraph::StopOnRoute
TransportCatalogueRouterGraph::StopOnRouteToDomain(const tc_serialize::StopOnRoutePB &stop) {
    TransportCatalogueRouterGraph::StopOnRoute result;

    auto st_name = tc_.GetStopNameById(stop.stop_id());
    const auto& [found, st_ref] = tc_.FindStop(st_name);
    result.stop_name = st_ref.stop_name;

    result.bus_name = stop.bus_name();
    result.stop_number = stop.stop_number();

    return result;
}

tc_serialize::TwoStopsLinkPB
TransportCatalogueRouterGraph::TwoStopsLinkToSerialize(const TwoStopsLink& link, graph::EdgeId edge) const {
    tc_serialize::TwoStopsLinkPB result;

    result.set_bus_name(std::string {link.bus_name});
    result.set_stop_from(link.stop_from);
    result.set_stop_to(link.stop_to);
    result.set_num_of_stops(link.number_of_stops);
    result.set_edge_id(edge);

    return std::move(result);
}

TwoStopsLink TransportCatalogueRouterGraph::TwoStopsLinkToDomain(const tc_serialize::TwoStopsLinkPB &link) const {
    TwoStopsLink result;

    const auto& b_name = link.bus_name();
    const auto& found = tc_.FindBus({b_name});
    result.bus_name = found.bus_name;

    result.number_of_stops = link.num_of_stops();
    result.stop_from = link.stop_from();
    result.stop_to = link.stop_to();

    return result;
}

tc_serialize::EdgePB TransportCatalogueRouterGraph::EdgeToSerialize(const graph::Edge<double> &edge) const {
    tc_serialize::EdgePB result;

    result.set_from(edge.from);
    result.set_to(edge.to);
    result.set_weight(edge.weight);

    return result;
}

graph::Edge<double> TransportCatalogueRouterGraph::EdgeToDomain(const tc_serialize::EdgePB &edge) const {
    graph::Edge<double> result{};

    result.from = edge.from();
    result.to = edge.to();
    result.weight = edge.weight();

    return result;
}

tc_serialize::IncListPB TransportCatalogueRouterGraph::IncListToSerialize(const std::vector<size_t>& list) const {
    tc_serialize::IncListPB result;

    for (const size_t edge : list) {
        result.add_list(edge);
    }

    return std::move(result);
}

std::vector<size_t> TransportCatalogueRouterGraph::IncListToDomain(const tc_serialize::IncListPB& list) const {
    std::vector<size_t> result(list.list_size());

    for (int i = 0; i < list.list_size(); ++i) {
        result[i] = list.list(i);
    }

    return std::move(result);
}
