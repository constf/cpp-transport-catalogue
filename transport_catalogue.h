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

namespace transport_catalogue {

struct Stop {
    std::string stop_name;
    Coordinates coordinates;
};

const Stop EMPTY_STOP{};

enum RouteType{
    NOT_SET,
    CIRCLE_ROUTE,
    WAY_AND_BACK_ROUTE
};

struct BusRoute{
    std::string bus_name;
    RouteType type;
    std::vector<const Stop*> route_stops;
};

const BusRoute EMPTY_BUS_ROUTE{};
const std::set<std::string_view> EMPTY_BUS_ROUTE_SET{};

struct BusInfo{
    std::string_view bus_name;
    RouteType type;
    size_t stops_number;
    size_t unique_stops;
    size_t route_length;
    double curvature;
};

std::ostream& operator<<(std::ostream&os, const BusInfo& bi);

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
    const Stop& AddStop(std::string& name, Coordinates coords);
    const Stop& AddStop(Stop& stop);
    std::pair<bool, const Stop&> FindStop(std::string_view name);
    const BusRoute& AddBus(BusRoute& bus_route);
    const BusRoute& FindBus(std::string_view name);
    BusInfo GetBusInfo(std::string_view bus_name);
    const std::set<std::string_view>& GetBusesForStop(std::string_view stop);
    bool SetDistanceBetweenStops(const std::string_view& stop, const std::string_view& other_stop, int dist);
    int GetDistanceBetweenStops(const std::string_view& stop, const std::string_view& other_stop);

private:
    std::deque<Stop> stops_;
    std::unordered_map<std::string_view, const Stop*> stops_index_;

    std::deque<BusRoute> bus_routes_;
    std::unordered_map<std::string_view, const BusRoute*> routes_index_;

    std::unordered_map<std::string_view, std::set<std::string_view>> stop_and_buses_;
    std::unordered_map<StopsPointers, int, StopsPointers, StopsPointers> stops_distance_index_;
};


} // transport_catalogue namespace