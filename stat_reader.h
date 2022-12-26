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

    size_t ParseRawIntoQueries(std::vector<Query>& output);

private:
    std::vector<std::pair<ObjectType, std::string>> raw_input_;
};


size_t ReadInputAndQueryTransportCatalogue(std::istream& input_stream, TransportCatalogue& tc);


} // namespace transport_catalogue