#include "map_renderer.h"
#include "domain.h"


svg::Point SphereProjector::operator()(geo::Coordinates coords) const {
    return {
            (coords.lng - min_lon_) * zoom_coeff_ + padding_,
            (max_lat_ - coords.lat) * zoom_coeff_ + padding_
    };
}

void MapRenderer::RenderSvgMap(const transport_catalogue::TransportCatalogue &tc, std::ostream& out) {
    // get all routes and all stops of the routes
    const std::map<std::string_view, const transport_catalogue::Stop*> stops = tc.GetAllStopsIndex();
    std::vector<geo::Coordinates> all_route_stops_coordinates;
    for (const auto& stop : stops) {
        if ( tc.GetBusesForStop(stop.first).empty() ) continue;
        all_route_stops_coordinates.push_back(stop.second->coordinates);
    }
    // now, initialize the SphereProjector object
    SphereProjector projector(all_route_stops_coordinates.begin(), all_route_stops_coordinates.end(),
                       settings_.width, settings_.height, settings_.padding);

    // create svg document that will contain all our svg objects for the routes map
    svg::Document svg_doc;
    const std::map<std::string_view, const transport_catalogue::BusRoute*> routes = tc.GetAllRoutesIndex();

    size_t color_count = 0;
    for (const auto route : routes) {
        if (route.second->route_stops.empty()) continue;
        svg::Color palette_color = GetNextPalleteColor(color_count);

        svg::Polyline line;
        line.SetStrokeColor(palette_color).SetFillColor({}).SetStrokeWidth(settings_.line_width)
        .SetStrokeLineCap(svg::StrokeLineCap::ROUND).SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

        for (auto iter = route.second->route_stops.begin(); iter != route.second->route_stops.end(); ++iter) {
            line.AddPoint(projector( (*iter)->coordinates ));
        }
        if (route.second->type == transport_catalogue::RouteType::RETURN_ROUTE) {
            for (auto back_iter = std::next(route.second->route_stops.rbegin()); back_iter != route.second->route_stops.rend(); ++back_iter) {
                line.AddPoint(projector( (*back_iter)->coordinates ));
            }
        }
        svg_doc.Add(std::move(line));
    }

    svg_doc.Render(out);
}

svg::Color MapRenderer::GetNextPalleteColor(size_t &color_count) const {
    if (color_count >= settings_.color_palette.size()) {
        color_count = 0;
    }
    return settings_.color_palette[color_count++];
}
