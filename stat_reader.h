#pragma once

#include <iostream>
#include <vector>
#include "input_reader.h"
#include "transport_catalogue.h"

namespace transport_catalogue {

struct Query {
    ObjectType query_type;
    std::string query_body;
};



class StatisticReaderAndOutput {
public:
    explicit StatisticReaderAndOutput(const TransportCatalogue& tc): transport_catalogue_(tc) {
    };
    size_t ReadInput_Query_AndPrint(std::istream& input, std::ostream& output_stream);

    void BusQueryAndPrint(std::string_view query_body, std::ostream& out);
    void StopQueryAndPrint(std::string_view query_body, std::ostream& out);

private:
    const TransportCatalogue& transport_catalogue_;
    std::vector<Query> raw_input_;
};


size_t ReadInputAndQueryTransportCatalogue(std::istream& input_stream, std::ostream& output_stream,TransportCatalogue& tc);


} // namespace transport_catalogue