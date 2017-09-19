#include <iostream>
#include <fstream>
#include <mapnik/datasource.hpp>
#include <mapnik/datasource_cache.hpp>
#include <mapnik/geometry.hpp>
#include <mapnik/geometry/boost_adapters.hpp>
#include <boost/geometry/algorithms/distance.hpp>
#include <boost/geometry/algorithms/comparable_distance.hpp>
#include <boost/geometry/extensions/algorithms/closest_point.hpp>
#include <boost/geometry/io/svg/svg_mapper.hpp>
#include <mapnik/util/geometry_to_wkt.hpp>

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
    boost::geometry::svg_mapper<mapnik::geometry::point<double>> mapper(svg, 600, 400,"width=\"800\" height=\"600\"");

    while (feature != nullptr)
    {

        auto const& geom = feature->get_geometry();
        if (geom.is<mapnik::geometry::polygon<double>>())
        {
            auto const& poly = geom.get<mapnik::geometry::polygon<double>>();
            mapper.add(poly);
            mapper.map(poly, "fill-opacity:0.3;fill:rgb(51,51,153);stroke:rgb(51,51,153);stroke-width:2");

            using result_type = boost::geometry::closest_point_result<mapnik::geometry::point<double>>;

            if (!poly.empty())
            {
                for (auto const& ring : poly)
                {
                    result_type info;
                    boost::geometry::closest_point(pt ,ring, info);
                    auto closest = info.closest_point;
                    std::string wkt;
                    mapnik::util::to_wkt(wkt, closest);
                    std::cerr << "closest point:" << wkt
                              << " distance=" << info.distance << std::endl;
                    mapper.add(closest);
                    mapper.map(closest,"fill-opacity:0.5;fill:green;stroke:green;stroke-width:2");
                }
            }
        }
        feature = features->next();
    }
    mapper.add(pt);
    mapper.map(pt,"fill-opacity:0.5;fill:red;stroke:red;stroke-width:2");
    std::cerr << "Done" << std::endl;
    return EXIT_SUCCESS;
}
