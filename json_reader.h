#pragma once

#include "json.h"
#include "transport_catalogue.h"
#include <vector>

const std::string BASE_DATA = "base_requests";
const std::string USER_REQUESTS = "stat_requests";

struct BusRouteJson {
    std::string bus_name;
    transport_catalogue::RouteType type;
    std::vector<std::string> route_stops;
};


using BaseRequest = std::variant<std::monostate, transport_catalogue::StopWithDistances, BusRouteJson>;


class JsonReader {
public:
    JsonReader(transport_catalogue::TransportCatalogue& tc) : transport_catalogue_(tc) {
    }

    size_t ReadJson(std::istream& input);

    size_t ReadJson_FillTransportCatalogue(std::istream& input);
    size_t QueryTC_WriteJsonToStream(std::ostream& out);

    size_t ReadJson_QueryTC_WriteJsonToStream(std::istream & input, std::ostream& out);


private:
    transport_catalogue::TransportCatalogue& transport_catalogue_;
    std::vector<json::Document> root_;
    std::vector<transport_catalogue::StopWithDistances> raw_stops_;
    std::vector<BusRouteJson> raw_buses_;

    BaseRequest ParseAddDataNode(const json::Node& node) const;
    size_t ParseJsonToRawData();
    bool FillTransportCatalogue();
    json::Node ProcessOneUserRequestNode(const json::Node& user_request);
};

inline json::Node GetErrorNode(int id);
