#pragma once

#include <sstream>
#include "json.h"
#include "transport_catalogue.h"
#include "transport_catalogue.pb.h"
#include "map_renderer.h"
#include "domain.h"
#include "router.h"
#include "transport_router.h"
#include <vector>
#include "serialization.h"


const std::string BASE_DATA = "base_requests";
const std::string USER_REQUESTS = "stat_requests";
const std::string RENDER_SETTINGS = "render_settings";
const std::string ROUTING_SETTINGS = "routing_settings";
const std::string SERIALIZE_SETTINGS = "serialization_settings";


struct BusRouteJson {
    std::string bus_name;
    transport_catalogue::RouteType type;
    std::vector<std::string> route_stops;
};


using BaseRequest = std::variant<std::monostate, transport_catalogue::StopWithDistances, BusRouteJson>;


class JsonReader {
public:
    explicit JsonReader(transport_catalogue::TransportCatalogue& tc) : transport_catalogue_(tc) {
    }

    size_t ReadJson(std::istream& input);

    size_t ReadJsonToTransportCatalogue(std::istream& input);
    size_t QueryTcWriteJsonToStream(std::ostream& out);

    size_t ReadJsonQueryTcWriteJsonToStream(std::istream & input, std::ostream& out);
    std::optional<graph::Router<double>::RouteInfo> GenerateRoute(std::string_view from_stop, std::string_view to_stop) const;

    [[nodiscard]] RendererSettings GetRendererSetting() const;
    RoutingSettings GetRoutingSettings() const;
    SerializationSettings GetSerializationSettings() const;

    void SaveTo(tc_serialize::TransportCatalogue& t_cat) const;
    bool RestoreFrom(tc_serialize::TransportCatalogue& t_cat);

private:
    transport_catalogue::TransportCatalogue& transport_catalogue_;
    std::vector<json::Document> root_;
    std::vector<transport_catalogue::StopWithDistances> raw_stops_;
    std::vector<BusRouteJson> raw_buses_;
    mutable std::optional<RoutingSettings> routing_settings_;
    mutable std::optional<RendererSettings> renderer_settings_;
    std::unique_ptr<TransportCatalogueRouterGraph> graph_ptr_;

    BaseRequest ParseDataNode(const json::Node& node) const;
    size_t ParseJsonToRawData();
    bool FillTransportCatalogue();
    json::Node ProcessOneUserRequestNode(const json::Node& user_request);
    std::optional<geo::Coordinates> ParseCoordinates(const json::Dict& dict) const;
    BaseRequest ParseDataStop(const json::Dict& dict) const;
    BaseRequest ParseDataBus(const json::Dict& dict) const;
    json::Node GenerateMapNode(int id) const;
    json::Node GenerateBusNode(int id, std::string& name) const;
    json::Node GenerateStopNode(int id, std::string& name) const;
    json::Node GenerateRouteNode(int id, std::string_view from, std::string_view to) const;
};

svg::Color ParseColor(const json::Node& node);
inline json::Node GetErrorNode(int id);
