#pragma once

#include <string>
#include "domain.h"
#include "transport_catalogue.pb.h"
#include "svg.h"


struct SerializationSettings {
    std::string file_name;
};

struct RendererSettings;
struct RoutingSettings;


tc_serialize::Stop SerializeStop(const transport_catalogue::Stop& stop);
tc_serialize::Coordinates SerializeCoordinates(const geo::Coordinates& coords);

transport_catalogue::Stop DeserializeStop(const tc_serialize::Stop& stop);
geo::Coordinates DeserializeCoordinates(const tc_serialize::Coordinates& coords);

tc_serialize::DistanceBetweenStops SerializeDistance(uint32_t from, uint32_t to, int distance);

tc_serialize::Point SerializePoint(const svg::Point& p);
svg::Point DeserializePoint(const tc_serialize::Point& p);

tc_serialize::RGB SerializeRgb(const svg::Rgb& rgb);
svg::Rgb DeserializeRgb(const tc_serialize::RGB& rgb);

tc_serialize::RGBA SerializeRgba(const svg::Rgba& rgba);
svg::Rgba DeserializeRgba(const tc_serialize::RGBA& rgba);

tc_serialize::Color SerializeColor(const svg::Color& color);
svg::Color DeserializeColor(const tc_serialize::Color& color);

tc_serialize::RenderSettings SerializeRendererSettings(const RendererSettings& settings);
RendererSettings DeserializeRenderSetting(const tc_serialize::RenderSettings& settings);

tc_serialize::RoutingSettings SerializeRouting(const RoutingSettings& settings);
RoutingSettings DeserializeRouting(const tc_serialize::RoutingSettings& settings);

// TransportCatalogueGraphRouter and graph::DirectWeightedGraph convertors are implemented as class methods
// graph::Router helper functions are defined in router.h