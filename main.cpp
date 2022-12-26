#include <iostream>
#include "transport_catalogue.h"
#include "input_reader.h"
#include "stat_reader.h"

using namespace transport_catalogue;


int main() {
    TransportCatalogue cat01;

    LoadStreamToTransportCatalogue(std::cin, cat01);
    ReadInputAndQueryTransportCatalogue(std::cin, cat01);

    return 0;
}
