#include "input_reader.h"
#include "stat_reader.h"

#include <string>
#include <iterator>
#include <iomanip>



namespace transport_catalogue {


size_t StatisticReaderAndOutput::ReadInput_Query_AndPrint(std::istream& input, std::ostream& output_stream) {
    const size_t number_of_queries = ReadLineWithNumber( input );

    for (size_t i = 0; i < number_of_queries; ++i) {
        // read raw line from input stream
        std::string raw_line = ReadLine(input );
        auto [type, first] = CheckTypeAndFindStart(raw_line);
        // make sure it is not empty or all blanks
        size_t last = raw_line.find_last_not_of(' ');
        if (last > raw_line.size()) continue;
        // single out the query body
        //std::string query_body = raw_line.substr(first, ++last - first);
        std::string_view query_body { raw_line.data() + first, ++last - first };

        if (type == ObjectType::BUS) {
            BusQueryAndPrint(query_body, output_stream);
        } else if (type == ObjectType::STOP) {
            StopQueryAndPrint(query_body, output_stream);
        } else {
            using namespace std::literals;
            std::cerr << "ReadInput_Query_AndPrint method: _ERROR while processing "s + raw_line + " raw data."s << std::endl;
        }
    }

    return number_of_queries;
}


void StatisticReaderAndOutput::BusQueryAndPrint(std::string_view query_body, std::ostream& out) {
    using namespace std::literals;
    BusInfo bi = transport_catalogue_.GetBusInfo(query_body);

    if (bi.type == RouteType::NOT_SET) {
        out << "Bus "s << query_body << ": not found"s << std::endl;
        return;
    }

    out << bi;
}

void StatisticReaderAndOutput::StopQueryAndPrint(std::string_view query_body, std::ostream &out) {
    using namespace std::literals;

    if ( ! transport_catalogue_.FindStop(query_body).first ) {
        out << "Stop "s << query_body << ": not found"s << std::endl;
        return;
    }
    const std::set<std::string_view>& bus_routes = transport_catalogue_.GetBusesForStop(query_body);
    if (bus_routes.empty()) {
        out << "Stop "s << query_body << ": no buses"s << std::endl;
        return;
    }
    out << "Stop "s << query_body << ": buses "s;
    for (auto iter = bus_routes.begin(); iter != bus_routes.end(); ++iter) {
        out << *iter;
        if (std::next(iter) != bus_routes.end()) {
            out << ' ';
        }
    }
    out << std::endl;
}


size_t ReadInputAndQueryTransportCatalogue(std::istream& input_stream, std::ostream& output_stream, TransportCatalogue& tc) {
    StatisticReaderAndOutput stat_reader(tc);

    size_t num_queries = stat_reader.ReadInput_Query_AndPrint(input_stream, output_stream);

//    const std::vector<Query>& queries = stat_reader.GetQueries();
//
//    for (const auto& q : queries) {
//        using namespace std::literals;
//        if (q.query_type == ObjectType::BUS){
//            BusInfo bi = tc.GetBusInfo(q.query_body);
//
//            if (bi.type == RouteType::NOT_SET) {
//                output_stream << "Bus "s << q.query_body << ": not found"s << std::endl;
//                continue;
//            }
//
//            output_stream << bi;
//        } else if (q.query_type == ObjectType::STOP) {
//            if ( ! tc.FindStop(q.query_body).first ) {
//                output_stream << "Stop "s << q.query_body << ": not found"s << std::endl;
//                continue;
//            }
//            const std::set<std::string_view>& bus_routes = tc.GetBusesForStop(q.query_body);
//            if (bus_routes.empty()) {
//                output_stream << "Stop "s << q.query_body << ": no buses"s << std::endl;
//                continue;
//            }
//            output_stream << "Stop "s << q.query_body << ": buses "s;
//            for (auto iter = bus_routes.begin(); iter != bus_routes.end(); ++iter) {
//                output_stream << *iter;
//                if (std::next(iter) != bus_routes.end()) {
//                    output_stream << ' ';
//                }
//            }
//            output_stream << std::endl;
//        } else {
//
//        }
//    }

    return num_queries;
}

}