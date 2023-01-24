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
    geo::Coordinates coordinates;
};

struct StopDistanceData {
    std::string other_stop_name;
    size_t distance;
};

struct StopWithDistances : Stop {
    std::vector<StopDistanceData> distances;
};

const Stop EMPTY_STOP{};

enum RouteType{
    NOT_SET,
    CIRCLE_ROUTE,
    RETURN_ROUTE
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
    void AddStop(const std::string& name, const geo::Coordinates coords);
    void AddStop(const Stop& stop);
    std::pair<bool, const Stop&> FindStop(const std::string_view name) const;
    bool AddBus(const BusRoute& bus_route);
    const BusRoute& FindBus(std::string_view name);
    BusInfo GetBusInfo(std::string_view bus_name) const;
    const std::set<std::string_view>& GetBusesForStop(std::string_view stop) const;
    bool SetDistanceBetweenStops(std::string_view stop, std::string_view other_stop, int dist);
    int GetDistanceBetweenStops(std::string_view stop, std::string_view other_stop) const;

private:
    std::deque<Stop> stops_;
    std::unordered_map<std::string_view, const Stop*> stops_index_;

    std::deque<BusRoute> bus_routes_;
    std::unordered_map<std::string_view, const BusRoute*> routes_index_;

    std::unordered_map<std::string_view, std::set<std::string_view>> stop_and_buses_;
    std::unordered_map<StopsPointers, int, StopsPointers, StopsPointers> stops_distance_index_;
};


} // transport_catalogue namespace