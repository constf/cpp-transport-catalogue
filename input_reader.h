#pragma once

#include <vector>
#include <set>
#include <string>
#include <iostream>

#include "transport_catalogue.h"

namespace transport_catalogue{


const std::string_view THIS_IS_STOP = "Stop";
const std::string_view THIS_IS_BUS = "Bus";

enum ObjectType {
    _ERROR,
    STOP,
    BUS
};


std::string ReadLine(std::istream &input);

int ReadLineWithNumber(std::istream &input);


class RawTransportData {
public:
    using BusAndStopesParsed = std::pair<RouteType, std::vector<std::string>>;

    RawTransportData() = default;

    size_t LoadRawData(std::istream &input);

    size_t LoadRawData(std::vector<std::string> &raw_strings);

    size_t ConvertRawDataToStops(std::vector<Stop> &stops_to_fill);

    size_t ParseRawDataToBusAndStopsStrings(std::vector<BusAndStopesParsed> &parsed_bus_stops_names);

    size_t ParseAndFillStopDistances(std::unordered_map<std::string_view, std::vector<StopDistanceData>>& distances);

    void ClearRawStopsData();

    void ClearRawBusRoutesData();

private:
    std::vector<std::string> raw_stops_; // storage for row quiries with stop info
    std::vector<std::string> raw_bus_routes_;  // storage for raw quiries with bus rote info
    std::unordered_map<std::string, std::string> stop_to_stops_; //strorage of raw data for stops distances between themselves
};


void LoadStreamToTransportCatalogue(std::istream &input_stream, TransportCatalogue &tc);

std::pair<ObjectType, size_t> CheckTypeAndFindStart(const std::string &line);

} // transport_catalogue namespace