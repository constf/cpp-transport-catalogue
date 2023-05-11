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


tc_serialize::Stop StopToSerialize(const transport_catalogue::Stop& stop);
tc_serialize::Coordinates CoordinatesToSerialize(const geo::Coordinates& coords);

transport_catalogue::Stop StopToDomain(const tc_serialize::Stop& stop);
geo::Coordinates CoordinatesToDomain(const tc_serialize::Coordinates& coords);

tc_serialize::DistanceBetweenStops DistanceToSerialize(uint32_t from, uint32_t to, int distance);

tc_serialize::Point PointToSerialize(const svg::Point& p);
svg::Point PointToDomain(const tc_serialize::Point& p);

tc_serialize::RGB RgbToSerialize(const svg::Rgb& rgb);
svg::Rgb RgbToDomain(const tc_serialize::RGB& rgb);

tc_serialize::RGBA RgbaToSerialize(const svg::Rgba& rgba);
svg::Rgba RgbaToDomain(const tc_serialize::RGBA& rgba);

tc_serialize::Color ColorToSerialize(const svg::Color& color);
svg::Color ColorToDomain(const tc_serialize::Color& color);

tc_serialize::RenderSettings RendererSettingsToSerialize(const RendererSettings& settings);
RendererSettings RenderSettingToDomain(const tc_serialize::RenderSettings& settings);

tc_serialize::RoutingSettings RoutingToSerialize(const RoutingSettings& settings);
RoutingSettings RoutingToDomain(const tc_serialize::RoutingSettings& settings);

// TransportCatalogueGraphRouter and graph::DirectWeightedGraph convertors are implemented as class methods
// graph::Router helper functions are defined in router.h