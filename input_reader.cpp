#include "input_reader.h"
#include <stdexcept>


namespace transport_catalogue{


size_t RawTransportData::LoadRawData(std::istream &input) {
    const size_t number_of_lines = ReadLineWithNumber(input);
    std::vector<std::string> raw_lines(number_of_lines);

    for (size_t i = 0; i < number_of_lines; ++i) {
        raw_lines[i] = ReadLine(input);
    }

    return LoadRawData(raw_lines);
}


size_t RawTransportData::LoadRawData(std::vector<std::string> &raw_strings) {
    const size_t number_of_lines = raw_strings.size();
    raw_stops_.reserve(number_of_lines);
    raw_bus_routes_.reserve(number_of_lines);

    for (const std::string &line: raw_strings) {
        auto [type, pos] = CheckTypeAndFindStart(line);

        if (type == ObjectType::STOP) {
            raw_stops_.push_back(line.substr(pos));
        } else if (type == ObjectType::BUS) {
            raw_bus_routes_.push_back(line.substr(pos));
        } else if (type == ObjectType::_ERROR) {
            using namespace std::literals;
            std::cerr << "LoadRawData method: _ERROR while processing "s + line + " raw data."s << std::endl;
        }
    }

    return number_of_lines;
}




size_t RawTransportData::ConvertRawDataToStops(std::vector<Stop> &stops_to_fill) {
    size_t count = 0;

    for (std::string &line: raw_stops_) {
        if (line.empty()) continue;
        Stop stop;
        size_t first = 0;
        using namespace std::literals;

        // single out the stop's name
        size_t last = line.find_first_of(":"s); // found the colon separator
        //
        // извините, не очень понял, что именно нужно сделать.
        //
        if (last < 1 || last == -1) continue;
        last = line.find_last_not_of(' ', last - 1); // check if there are some spaces before colon, skip them
        if (last == -1) continue;
        stop.stop_name = line.substr(first, ++last - first);

        try {
            // single out the lattitude
            first = line.find_first_not_of(": "s, last);
            last = line.find_first_of(", "s, first);
            stop.coordinates.lat = std::stod(line.substr(first, last - first));

            // single out the longitude
            first = line.find_first_not_of(", "s, last);
            last = line.find_first_of(", ", first);
            stop.coordinates.lng = stod(line.substr(first, last - first));
        } catch (std::invalid_argument &e) {
            std::cerr << "Error while converting raw data coordinates for Stop with name: "s << stop.stop_name
                      << std::endl;
            std::cerr << "Error details: "s << e.what() << std::endl;
            continue;
        } catch (std::out_of_range &e) {
            std::cerr << "Error while converting raw data coordinates for Stop with name: "s << stop.stop_name
                      << std::endl;
            std::cerr << "Error details: "s << e.what() << std::endl;
            continue;
        }
        stops_to_fill.push_back(stop);

        if (last > line.size()) { // no more symbols in this line, no distance data here. create empty map record
            stop_to_stops_[stop.stop_name];
            continue;
        }

        first = line.find_first_not_of(", ", last); // stop at the first symbol of distance data to other stops

        stop_to_stops_.emplace(stop.stop_name, line.substr(first, last - first)); // add to container for later parsing, when all stops are registered in this for-loop

        ++count;
    }

    return count;
}


size_t RawTransportData::ParseAndFillStopDistances(std::unordered_map<std::string_view, std::vector<StopDistanceData>> &distances) {
    size_t count = 0;

    for (auto& [stop_name, line] : stop_to_stops_) {
        std::vector<StopDistanceData> sdd;
        size_t first = 0;
        size_t last = 0;
        const size_t SIZE = line.size();
        for ( last = line.find_first_of('m', first); first < SIZE && last < SIZE; ) {
            size_t dist = std::stol(std::string {line.substr(first, last - first)}); // read the distance
            first = line.find_first_of('o', last) + 1; // next space after "to"
            first = line.find_first_not_of(' ', first); // stop name first symbol
            last = line.find_first_of(',', first);  // separator after stop name
            last = line.find_last_not_of(' ', last - 1); // check if there are some spaces before separator, skip them
            sdd.push_back( {line.substr(first, ++last - first), dist} );

            first = line.find_first_not_of(", ", last);
            last = line.find_first_of('m', first);
        }

        distances.emplace(stop_name, sdd);
        ++count;
    }

    return count;
}





size_t RawTransportData::ParseRawDataToBusAndStopsStrings(std::vector<BusAndStopesParsed> &parsed_bus_stops_names) {
    size_t count = 0;

    for (std::string &line: raw_bus_routes_) {
        transport_catalogue::RouteType type = RouteType::NOT_SET;
        std::vector<std::string> bus_stops; // front element is bus name, others - stops' names

        size_t first = 0;
        size_t last = line.find_first_of(':');
        if (last < 1 || last == -1) continue;
        last = line.find_last_not_of(' ', last - 1); // check if there are some spaces before colon, skip them
        if (last == -1) continue;
        bus_stops.push_back(line.substr(first, ++last - first));

        const size_t SIZE = line.size();
        for (first = line.find_first_not_of(": ", last); first < SIZE && last < SIZE;) {
            last = line.find_first_of(">-", first); // found the separator between stops
            if (last < SIZE && type == RouteType::NOT_SET) {
                type = line[last] == '>' ? RouteType::CIRCLE_ROUTE : RouteType::RETURN_ROUTE;
            }
            last = line.find_last_not_of(' ', last - 1); // check if there are some spaces before separator, skip them
            bus_stops.push_back(line.substr(first, ++last - first));

            first = line.find_first_not_of(">- ", last);
        }
        parsed_bus_stops_names.emplace_back(BusAndStopesParsed{type, bus_stops});
        ++count;
    }

    return count;
}

void RawTransportData::ClearRawStopsData() {
    raw_stops_.clear();
}

void RawTransportData::ClearRawBusRoutesData() {
    raw_bus_routes_.clear();
}


std::string ReadLine(std::istream &input) {
    std::string s;
    std::getline(input, s);
    return s;
}


int ReadLineWithNumber(std::istream &input) {
    int result;
    input >> result;
    ReadLine(input);
    return result;
}



std::pair<ObjectType, size_t> CheckTypeAndFindStart(const std::string &line) {
    size_t first = line.find_first_not_of(' ');
    size_t last = line.find_first_of(' ', first);
    std::string data_type = line.substr(first, last - first);

    ObjectType result;
    if (data_type == THIS_IS_BUS) {
        result = ObjectType::BUS;
    } else if (data_type == THIS_IS_STOP) {
        result = ObjectType::STOP;
    } else {
        result = ObjectType::_ERROR;
    }

    first = line.find_first_not_of(' ', last);

    return {result, first};
}


void LoadStreamToTransportCatalogue(std::istream &input_stream, TransportCatalogue &tc) {
    RawTransportData raw_data;
    raw_data.LoadRawData(input_stream);

    std::vector<Stop> stops;
    raw_data.ConvertRawDataToStops(stops);

    std::unordered_map<std::string_view, std::vector<StopDistanceData>> distances;
    raw_data.ParseAndFillStopDistances(distances);

    std::vector<RawTransportData::BusAndStopesParsed> raw_bus_routes;
    raw_data.ParseRawDataToBusAndStopsStrings(raw_bus_routes);

    // added all stops
    for (auto& stop : stops) {
        tc.AddStop(stop);
    }
    // now add distances between them
    for (const auto& [stop, distances_data] : distances) {
        for (const auto& [other, dist] : distances_data) {
            bool pairOK = tc.SetDistanceBetweenStops(stop, other, dist);
            if ( !pairOK ) {
                using namespace std::literals;
                std::cerr << "ERROR while adding distance to stop pair of "s << stop << " and "s << other << "." << std::endl;
            }
        }
    }

    using namespace std::literals;
    for (auto& raw_bus : raw_bus_routes) {
        const size_t SIZE = raw_bus.second.size();
        if (SIZE < 3) {
            std::cerr << "Error while adding bus routes for bus: "s << raw_bus.second.front() << ". Number of stops must be at least 2." << std::endl;
            continue;
        }
        BusRoute br;
        br.bus_name = std::move(raw_bus.second[0]);
            br.type = raw_bus.first;
        br.route_stops.resize(SIZE - 1);
        for(size_t i = 1; i < SIZE; ++i) {
            br.route_stops[i - 1] = &(tc.FindStop(raw_bus.second[i]).second);
        }
        tc.AddBus(br);
    }
}

}