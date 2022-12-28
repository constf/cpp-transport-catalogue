#include "input_reader.h"
#include "stat_reader.h"

#include <string>
#include <iterator>
#include <iomanip>



namespace transport_catalogue {

std::ostream& operator<<(std::ostream& os, const BusInfo& bi) {
    using namespace std::literals;
    double length = bi.route_length;

    os  << "Bus "s << bi.bus_name << ": "s << bi.stops_number << " stops on route, "s
        << bi.unique_stops << " unique stops, "s << std::setprecision(6) << length << " route length, "s
        << std::setprecision(6) << bi.curvature << " curvature"s << std::endl;

    return os;
}




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
        auto [type, first] = CheckTypeAndFindStart(line);
        size_t last = line.find_last_not_of(' ');
        if (last > line.size()) continue;

        if (type == ObjectType::BUS) {
            raw_input_.emplace_back(Query {ObjectType::BUS, line.substr(first, ++last - first)});
        } else if (type == ObjectType::STOP) {
            raw_input_.emplace_back(Query {ObjectType::STOP, line.substr(first, ++last - first)});
        } else {
            using namespace std::literals;
            std::cerr << "ReadInputQueries method: _ERROR while processing "s + line + " raw data."s << std::endl;
            raw_input_.emplace_back(Query {ObjectType::_ERROR, line});
        }
    }

    return number_of_lines;
}


const std::vector<Query>& StatisticReaderAndOutput::GetQueries() const{
    return raw_input_;
}


size_t ReadInputAndQueryTransportCatalogue(std::istream& input_stream, std::ostream& output_stream, TransportCatalogue& tc) {
    StatisticReaderAndOutput stat_reader;
    stat_reader.ReadInputQueries(input_stream);

    const std::vector<Query>& queries = stat_reader.GetQueries();

    for (const auto& q : queries) {
        using namespace std::literals;
        if (q.query_type == ObjectType::BUS){
            BusInfo bi = tc.GetBusInfo(q.query_body);

            if (bi.type == RouteType::NOT_SET) {
                output_stream << "Bus "s << q.query_body << ": not found"s << std::endl;
                continue;
            }

            output_stream << bi;
        } else if (q.query_type == ObjectType::STOP) {
            if ( ! tc.FindStop(q.query_body).first ) {
                output_stream << "Stop "s << q.query_body << ": not found"s << std::endl;
                continue;
            }
            const std::set<std::string_view>& bus_routes = tc.GetBusesForStop(q.query_body);
            if (bus_routes.empty()) {
                output_stream << "Stop "s << q.query_body << ": no buses"s << std::endl;
                continue;
            }
            output_stream << "Stop "s << q.query_body << ": buses "s;
            for (auto iter = bus_routes.begin(); iter != bus_routes.end(); ++iter) {
                output_stream << *iter;
                if (std::next(iter) != bus_routes.end()) {
                    output_stream << ' ';
                }
            }
            output_stream << std::endl;
        } else {

        }
    }

    return queries.size();
}

}