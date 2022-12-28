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
    bool AddStop(const std::string& name, const Coordinates coords);
    bool AddStop(const Stop& stop);
    std::pair<bool, const Stop&> FindStop(const std::string_view name) const;
    // По поводу optional: optional не позволяет использовать reference класс. Вот выдержка из класса optional, последняя строка в комментарии
    // nullptr не передать, т.к. это ссылка, а не указатаель.
    // Хотя, наверное, как-то можно. Я не нашёл. Если можно, расскажите ПОЖАЛУЙСТА!!!
    // Оставил как было до дальнейших указаний.
    //  template<typename _Tp>
    //    class optional
    //    : private _Optional_base<_Tp>,
    //      private _Enable_copy_move<
    //	// Copy constructor.
    //	is_copy_constructible_v<_Tp>,
    //	// Copy assignment.
    //	__and_v<is_copy_constructible<_Tp>, is_copy_assignable<_Tp>>,
    //	// Move constructor.
    //	is_move_constructible_v<_Tp>,
    //	// Move assignment.
    //	__and_v<is_move_constructible<_Tp>, is_move_assignable<_Tp>>,
    //	// Unique tag type.
    //	optional<_Tp>>
    //    {
    //      static_assert(!is_same_v<remove_cv_t<_Tp>, nullopt_t>);
    //      static_assert(!is_same_v<remove_cv_t<_Tp>, in_place_t>);
    //      static_assert(!is_reference_v<_Tp>);

    bool AddBus(const BusRoute& bus_route);
    const BusRoute& FindBus(std::string_view name);
    BusInfo GetBusInfo(std::string_view bus_name);
    const std::set<std::string_view>& GetBusesForStop(std::string_view stop);
    bool SetDistanceBetweenStops(const std::string_view stop, const std::string_view other_stop, int dist);
    int GetDistanceBetweenStops(const std::string_view stop, const std::string_view other_stop);

private:
    std::deque<Stop> stops_;
    std::unordered_map<std::string_view, const Stop*> stops_index_;

    std::deque<BusRoute> bus_routes_;
    std::unordered_map<std::string_view, const BusRoute*> routes_index_;

    std::unordered_map<std::string_view, std::set<std::string_view>> stop_and_buses_;
    std::unordered_map<StopsPointers, int, StopsPointers, StopsPointers> stops_distance_index_;
};


} // transport_catalogue namespace