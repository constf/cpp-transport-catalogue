#include <iostream>
#include "transport_catalogue.h"
#include "input_reader.h"
#include "stat_reader.h"

using namespace transport_catalogue;


int main() {
    TransportCatalogue catalogue01;

    LoadStreamToTransportCatalogue(std::cin, catalogue01);
    ReadInputAndQueryTransportCatalogue(std::cin, std::cout, catalogue01);

    return 0;
}
