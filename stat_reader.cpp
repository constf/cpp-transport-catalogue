#include "input_reader.h"
#include "stat_reader.h"

#include <string>
#include <iterator>

namespace transport_catalogue {

size_t StatisticReaderAndOutput::ReadInputQueries(std::istream &input) {
    const size_t number_of_queries = ReadLineWithNumber( input );
    std::vector<std::string> raw_lines( number_of_queries );

    for (size_t i = 0; i < number_of_queries; ++i) {
        raw_lines[i] = ReadLine( input );
    }

    return ReadInputQueries(raw_lines);
}


size_t StatisticReaderAndOutput::ReadInputQueries(std::vector<std::string> &raw_strings) {
    const size_t number_of_lines = raw_strings.size();
    raw_input_.reserve(number_of_lines);

    for (const std::string& line : raw_strings) {
        auto [type, pos] = CheckTypeAndFindStart(line);

        if (type == ObjectType::BUS) {
            raw_input_.emplace_back( ObjectType::BUS, line.substr(pos) );
        } else if (type == ObjectType::STOP) {
            raw_input_.emplace_back( ObjectType::STOP, line.substr(pos) );
        } else {
            using namespace std::literals;
            std::cerr << "ReadInputQueries method: _ERROR while processing "s + line + " raw data."s << std::endl;
            raw_input_.emplace_back( ObjectType::_ERROR, line );
        }
    }

    return number_of_lines;
}

size_t StatisticReaderAndOutput::ParseRawIntoQueries(std::vector<Query> &output) {
    size_t count = 0;

    for (const auto & [type, line] : raw_input_) {
        if (line.empty()) continue;
        Query query;
        size_t first = 0;
        size_t last = line.find_last_not_of(' ');
        if (last > line.size()) continue;
        query.query_body = line.substr(first, ++last - first);
        query.query_type = type;
        output.push_back(query);
        ++count;
    }

    return count;
}




size_t ReadInputAndQueryTransportCatalogue(std::istream& input_stream, TransportCatalogue& tc) {
    StatisticReaderAndOutput stat_reader;
    stat_reader.ReadInputQueries(input_stream);

    std::vector<Query> queries;
    stat_reader.ParseRawIntoQueries(queries);

    for (const auto& q : queries) {
        using namespace std::literals;
        if (q.query_type == ObjectType::BUS){
            BusInfo bi = tc.GetBusInfo(q.query_body);

            if (bi.type == RouteType::NOT_SET) {
                std::cout << "Bus "s << q.query_body << ": not found"s << std::endl;
                continue;
            }

            std::cout << bi;
        } else if (q.query_type == ObjectType::STOP) {
            if ( ! tc.FindStop(q.query_body).first ) {
                std::cout << "Stop "s << q.query_body << ": not found"s << std::endl;
                continue;
            }
            const std::set<std::string_view>& bus_routes = tc.GetBusesForStop(q.query_body);
            if (bus_routes.empty()) {
                std::cout << "Stop "s << q.query_body << ": no buses"s << std::endl;
                continue;
            }
            std::cout << "Stop "s << q.query_body << ": buses "s;
            for (auto iter = bus_routes.begin(); iter != bus_routes.end(); ++iter) {
                std::cout << *iter;
                if (std::next(iter) != bus_routes.end()) {
                    std::cout << ' ';
                }
            }
            std::cout << std::endl;
        } else {

        }
    }

    return queries.size();
}

}