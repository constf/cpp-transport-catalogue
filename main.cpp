#include <iostream>
#include <fstream>
#include "transport_catalogue.h"
#include "json_reader.h"
#include "map_renderer.h"

using namespace transport_catalogue;
using namespace std::literals;


void PrintUsage(std::ostream& stream = std::cerr) {
    stream << "Usage: transport_catalogue [make_base|process_requests]\n"sv;
}


int main(int argc, char* argv[]) {
    TransportCatalogue tc;
    JsonReader reader(tc);
//    reader.ReadJsonToTransportCatalogue(std::cin);
//    reader.QueryTcWriteJsonToStream(std::cout);
//    return 0;

    if (argc != 2) {
        PrintUsage();
        return 1;
    }

    const std::string_view mode(argv[1]);

    if (mode == "make_base"sv) {
        reader.ReadJsonToTransportCatalogue(std::cin);

        SerializationSettings settings = reader.GetSerializationSettings();
        std::ofstream output_file(settings.file_name, std::ios::binary | std::ios::out);
        tc_serialize::TransportCatalogue t_cat;

        tc.SaveTo(t_cat);
        reader.SaveTo(t_cat);

        t_cat.SerializeToOstream(&output_file);
    } else if (mode == "process_requests"sv) {
        reader.ReadJson(std::cin);
        tc_serialize::TransportCatalogue t_cat;

        SerializationSettings settings = reader.GetSerializationSettings();
        std::ifstream input_file(settings.file_name, std::ios::binary | std::ios::in);

        t_cat.ParseFromIstream(&input_file);

        tc.RestoreFrom(t_cat);
        reader.RestoreFrom(t_cat);

        reader.QueryTcWriteJsonToStream(std::cout);
    } else {
        PrintUsage();
        return 1;
    }
}
