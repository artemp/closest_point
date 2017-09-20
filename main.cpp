#include <iostream>
#include <fstream>
#include <mapnik/datasource.hpp>
#include <mapnik/datasource_cache.hpp>
#include <mapnik/geometry.hpp>
#include <mapnik/geometry/boost_adapters.hpp>
#include <boost/geometry/algorithms/within.hpp>
#include <boost/geometry/algorithms/distance.hpp>
#include <boost/geometry/algorithms/comparable_distance.hpp>
#include <boost/geometry/extensions/algorithms/closest_point.hpp>
#include <boost/geometry/io/svg/svg_mapper.hpp>
#include <mapnik/util/geometry_to_wkt.hpp>

namespace mapnik { namespace geometry {

struct closest_point
{
    using result_type = boost::geometry::closest_point_result<mapnik::geometry::point<double>>;

    closest_point(mapnik::geometry::point<double> const& pt)
        : pt_(pt) {}

    result_type operator() (mapnik::geometry::geometry_empty const&) const
    {
        return result_type(); // FIXME: consider std::optional<result_type>
    }

    result_type operator() (mapnik::geometry::point<double> const& pt) const
    {
        result_type info;
        boost::geometry::closest_point(pt_ ,pt, info);
        return info;
    }

    result_type operator() (mapnik::geometry::line_string<double> const& line) const
    {
        result_type info;
        boost::geometry::closest_point(pt_ ,line, info);
        return info;
    }

    result_type operator() (mapnik::geometry::polygon<double> const& poly) const
    {
        result_type info;
        if (boost::geometry::within(pt_, poly))
        {
            info.closest_point = pt_;
            info.distance = 0.0;
            return info;
        }
        bool first = true;
        for (auto const& ring : poly)
        {
            result_type ring_info;
            boost::geometry::closest_point(pt_ ,ring, ring_info);
            if (first)
            {
                first = false;
                info = std::move(ring_info);
            }
            else if (ring_info.distance < info.distance)
            {
                info = std::move(ring_info);
            }
        }
        return info;
    }



    // Multi* + GeometryCollection
    result_type operator() (mapnik::geometry::geometry<double> const& geom) const
    {
        return mapnik::util::apply_visitor(*this, geom);
    }

    template <typename T>
    result_type operator() (T const& multi_geom) const
    {
        result_type info;
        bool first = true;
        for (auto const& geom : multi_geom)
        {
            if (first)
            {
                first = false;
                info = std::move(operator()(geom));
            }
            else
            {
                auto sub_info = operator()(geom);
                if (sub_info.distance < info.distance)
                {
                    info = std::move(sub_info);
                }
            }
        }
        return info;
    }

    mapnik::geometry::point<double> pt_;
};

}}

int main(int argc, char** argv)
{
    std::cerr << "Comparable distance test" << std::endl;
    mapnik::datasource_cache::instance().register_datasources("/opt/mapnik/lib/mapnik/input");
    if (argc != 4)
    {
        std::cerr << "Usage: <geojson-filename> <x> <y>" << std::endl;
        return EXIT_FAILURE;
    }
    mapnik::parameters params;
    params["type"] = "geojson";
    params["file"] = std::string(argv[1]);
    double x = std::stod(argv[2]);
    double y = std::stod(argv[3]);
    auto ds = mapnik::datasource_cache::instance().create(params);
    if (ds == nullptr)
    {
        std::cerr << "Failed to create datasource" << std::endl;
        return EXIT_FAILURE;
    }
    mapnik::query q(ds->envelope());
    auto features = ds->features(q);
    auto feature =  features->next();

    mapnik::geometry::point<double> pt(x, y);

    std::ofstream svg("point_to_geometry_distance.svg");
    boost::geometry::svg_mapper<mapnik::geometry::point<double>> mapper(svg, 600, 400,
                                                                        "width=\"800\" height=\"600\" fill=\"white\"");

    std::array<std::string, 4> four_colors = {{"red", "blue", "green", "yellow"}};
    std::size_t counter = 0;
    while (feature != nullptr)
    {
        auto const& geom = feature->get_geometry();
        if (geom.is<mapnik::geometry::polygon<double>>())
        {

            auto const& poly = geom.get<mapnik::geometry::polygon<double>>();
            mapper.add(poly);
            std::string style = "stroke:blue;stroke-width:1; fill-opacity:0.5;fill:" + four_colors[counter % four_colors.size()];
            ++counter;
            mapper.map(poly, style);
        }

        auto result = mapnik::util::apply_visitor(mapnik::geometry::closest_point(pt), geom);
        std::string wkt;
        mapnik::util::to_wkt(wkt, result.closest_point);
        std::cerr << "closest point:" << wkt
                  << " distance=" << result.distance << std::endl;

        mapnik::geometry::line_string<double> line;
        line.push_back(pt);
        line.push_back(result.closest_point);
        mapper.add(line);
        mapper.map(line,"stroke:red;stroke-width:1;stroke-dasharray:4,4;comp-op:color-burn");

        mapper.add(result.closest_point);
        mapper.map(result.closest_point,"fill-opacity:1.0;fill:yellow;stroke:blue;stroke-width:1");
        feature = features->next();
    }
    mapper.add(pt);
    mapper.map(pt,"fill-opacity:1.0;fill:salmon;stroke:blue;stroke-width:1");
    std::cerr << "Done" << std::endl;
    return EXIT_SUCCESS;
}
