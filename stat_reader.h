#pragma once

#include <iostream>
#include <vector>
#include "input_reader.h"

namespace transport_catalogue {

struct Query {
    ObjectType query_type;
    std::string query_body;
};



class StatisticReaderAndOutput {
public:
    StatisticReaderAndOutput() = default;
    size_t ReadInputQueries(std::istream& input);
    size_t ReadInputQueries(std::vector<std::string>& raw_strings);

    [[nodiscard]] const std::vector<Query>& GetQueries() const;

private:
    std::vector<Query> raw_input_;
};


size_t ReadInputAndQueryTransportCatalogue(std::istream& input_stream, std::ostream& output_stream,TransportCatalogue& tc);


} // namespace transport_catalogue