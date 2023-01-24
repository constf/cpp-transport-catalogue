#include "json_reader.h"


size_t JsonReader::ReadJson(std::istream &input) {
    size_t result = 0;

    try {
        json::Document doc = json::Load(input);
        if (doc.GetRoot().IsMap()) {
            result = doc.GetRoot().AsMap().size();
            root_.emplace_back(std::move(doc));
        }
    } catch (const json::ParsingError& e) {
        std::cerr << e.what() << std::endl;
    }

    return result;
}

size_t JsonReader::ReadJson_FillTransportCatalogue(std::istream &input) {
    size_t check_num = ReadJson(input);
    if (check_num == 0) return check_num;

    size_t result = ParseJsonToRawData();

    FillTransportCatalogue();

    return result;
}

size_t JsonReader::ParseJsonToRawData() {
    size_t result = 0;

    const json::Node& root_node = root_.back().GetRoot();
    if (!root_node.IsMap()) {
        throw json::ParsingError("Error reading JSON data for database filling.");
    }
    const json::Dict& dict = root_node.AsMap();
    auto iter = dict.find(BASE_DATA);
    if (iter == dict.end() || !(iter->second.IsArray()) ) {
        throw json::ParsingError("Error reading JSON data for database filling.");
    }

    const json::Array& nodes = iter->second.AsArray();
    for (const auto& node : nodes) {
        using namespace transport_catalogue;
        BaseRequest data = std::move(ParseAddDataNode(node));
        if (auto* stop = std::get_if<StopWithDistances>(&data)) {
            raw_stops_.emplace_back(std::move(*stop));
        } else if (auto* bus = std::get_if<BusRouteJson>(&data) ) {
            raw_buses_.emplace_back(std::move(*bus));
        } else {
            throw json::ParsingError("Error reading JSON data for database filling.");
        }
        ++result;
    }

    return result;
}



BaseRequest JsonReader::ParseAddDataNode(const json::Node &node) const {
    using namespace std::literals;
    using namespace transport_catalogue;
    if (!node.IsMap()) return {};
    const json::Dict& dict = node.AsMap();
    const auto type_i = dict.find("type"s);
    if (type_i == dict.end()) return {};
    if (type_i->second == "Stop"s) {
        StopWithDistances stop;

        if (const auto name_i = dict.find("name"s); name_i != dict.end() && name_i->second.IsString()) {
            stop.stop_name = name_i->second.AsString();
        } else return {};

        if (const auto lat_i = dict.find("latitude"s); lat_i != dict.end() && lat_i->second.IsDouble()) {
            stop.coordinates.lat = lat_i->second.AsDouble();
        } else return {};

        if (const auto lng_i = dict.find("longitude"s); lng_i != dict.end() && lng_i->second.IsDouble()) {
            stop.coordinates.lng = lng_i->second.AsDouble();
        } else return {};

        const auto dist_i = dict.find("road_distances"s);
        if (dist_i != dict.end() && !(dist_i->second.IsMap())) return {}; // проверка, что это словарь. он необязательный для остановки.
        for (const auto& [other_name, other_dist] : dist_i->second.AsMap()) {
            if (!other_dist.IsInt()) return {};
            stop.distances.emplace_back(StopDistanceData{other_name, static_cast<size_t>(other_dist.AsInt())});
        }
        return {stop};

    } else if (type_i->second == "Bus"s) {
        BusRouteJson route;

        if (const auto name_i = dict.find("name"s); name_i != dict.end() && name_i->second.IsString()) {
            route.bus_name = name_i->second.AsString();
        } else return {};
        if (const auto route_i = dict.find("is_roundtrip"s); route_i != dict.end() && route_i->second.IsBool()) {
            route.type = route_i->second.AsBool() ? RouteType::CIRCLE_ROUTE : RouteType::RETURN_ROUTE;
        } else return {};

        const auto stops_i = dict.find("stops"s);
        if (stops_i != dict.end() && !(stops_i->second.IsArray())) return {};
        for (const auto& stop_name : stops_i->second.AsArray()) {
            if (!stop_name.IsString()) return {};
            route.route_stops.emplace_back(stop_name.AsString());
        }

        return {route};

    } else {
        return {};
    }
}

bool JsonReader::FillTransportCatalogue() {

    // fill stops data to catalogue
    for (const auto& stop : raw_stops_) {
        transport_catalogue_.AddStop(stop);
    }

    // fill stop distances to catalogue
    for (const auto& stop : raw_stops_) {
        for (const auto& [other, distance] : stop.distances) {
            bool pairOK = transport_catalogue_.SetDistanceBetweenStops(stop.stop_name, other, distance);
            if ( !pairOK ) {
                using namespace std::literals;
                std::cerr << "ERROR while adding distance to stop pair of "s << stop.stop_name << " and "s << other << "." << std::endl;
            }
        }
    }

    // fill bus routes data to catalogue
    for (auto& route : raw_buses_) {
        using namespace std::literals;

        if (route.route_stops.size() < 2) {
            std::cerr << "Error while adding bus routes for bus: "s << route.bus_name << ". Number of stops must be at least 2." << std::endl;
            continue;
        }
        transport_catalogue::BusRoute br;
        br.bus_name = std::move(route.bus_name);
        br.type = route.type;
        for (auto& route_stop : route.route_stops) {
            br.route_stops.emplace_back( &(transport_catalogue_.FindStop(route_stop).second) );
        }

        transport_catalogue_.AddBus(br);
    }

    return true;
}


size_t JsonReader::QueryTC_WriteJsonToStream(std::ostream &out) {
    const auto& root_node = root_.back().GetRoot();
    if (!root_node.IsMap()) {
        throw json::ParsingError("Error reading JSON data with user requests to database.");
    }

    const json::Dict& dict = root_node.AsMap();
    auto iter = dict.find(USER_REQUESTS);
    if (iter == dict.end() || !(iter->second.IsArray()) ) {
        throw json::ParsingError("Error reading JSON data with user requests to database.");
    }

    json::Array result;
    for (const json::Node& node : iter->second.AsArray()) {
        if (!node.IsMap()) {
            throw json::ParsingError("Error reading JSON data with user requests to database. One of nodes is not a dictionary.");
        }

        result.emplace_back(std::move(ProcessOneUserRequestNode(node)));
    }
    json::PrintNode(result, out);

    return result.size();
}

size_t JsonReader::ReadJson_QueryTC_WriteJsonToStream(std::istream &input, std::ostream &out) {
    ReadJson(input);
    return QueryTC_WriteJsonToStream(out);
}

json::Node JsonReader::ProcessOneUserRequestNode(const json::Node &user_request) {
    using namespace std::literals;
    using namespace transport_catalogue;

    if (!user_request.IsMap()) {
        throw json::ParsingError("Error reading JSON data with user requests to database. One of nodes is not a dictionary.");
    }
    const json::Dict& request_fields = user_request.AsMap();

    int id = -1;
    if (const auto id_i = request_fields.find("id"s); id_i != request_fields.end() && id_i->second.IsInt()) {
        id = id_i->second.AsInt();
    } else throw json::ParsingError("Error reading JSON data with user requests to database. One of node's fields is crippled.");

    std::string name;
    if (const auto name_i = request_fields.find("name"s); name_i != request_fields.end() && name_i->second.IsString()) {
        name = name_i->second.AsString();
    } else throw json::ParsingError("Error reading JSON data with user requests to database. One of node's fields is crippled.");

    const auto type_i = request_fields.find("type"s);
    if ( type_i == request_fields.end() || !(type_i->second.IsString()) ){
        throw json::ParsingError("Error reading JSON data with user requests to database. One of node's fields is crippled.");
    }
    std::string type = type_i->second.AsString();
    if ( type == "Bus"s) {
        BusInfo bi = transport_catalogue_.GetBusInfo(name);
        if (bi.type == RouteType::NOT_SET) {
            return GetErrorNode(id);
        }
        json::Dict result;
        result.emplace("request_id"s, id);
        result.emplace("curvature"s, bi.curvature);
        result.emplace("route_length"s, static_cast<int>(bi.route_length));
        result.emplace("stop_count"s, static_cast<int>(bi.stops_number));
        result.emplace("unique_stop_count"s, static_cast<int>(bi.unique_stops));

        return {result};

    } else if (type == "Stop"s) {
        if ( ! transport_catalogue_.FindStop(name).first ) {
            return GetErrorNode(id);
        }
        json::Dict result;
        json::Array buses;
        const std::set<std::string_view>& bus_routes = transport_catalogue_.GetBusesForStop(name);
        for (auto bus_route : bus_routes) {
            buses.emplace_back( std::move(std::string{bus_route}) );
        }
        result.emplace("request_id"s, id);
        result.emplace("buses"s, buses);

        return {result};

    } else {
        throw json::ParsingError("Error reading JSON data with user requests to database. Node's type field contains invalid data.");
    }

}

inline json::Node GetErrorNode(int id) {
    using namespace std::literals;
    json::Dict result;
    result.emplace("request_id"s, id);
    result.emplace("error_message"s, "not found"s);

    return {result};
}
