#include <iostream>
#include <fstream>
#include <mapnik/datasource.hpp>
#include <mapnik/datasource_cache.hpp>
#include <mapnik/geometry.hpp>
#include <mapnik/geometry/boost_adapters.hpp>
#include <boost/geometry/algorithms/distance.hpp>
#include <boost/geometry/algorithms/comparable_distance.hpp>
#include <boost/geometry/extensions/algorithms/distance_info.hpp>
#include <boost/geometry/io/svg/svg_mapper.hpp>
#include <mapnik/util/geometry_to_wkt.hpp>

namespace {

struct comparable_distance
{
    comparable_distance(mapnik::geometry::point<double> const& pt)
        : pt_(pt) {}

    double operator() (mapnik::geometry::geometry_empty const&) const
    {
        return 0;
    }
    double operator() (mapnik::geometry::geometry_collection<double> const&) const
    {
        return 0;
    }
    template <typename T>
    double operator() (T const& geom) const
    {
        return boost::geometry::comparable_distance(pt_, geom);
    }
    mapnik::geometry::point<double> pt_;
};

struct distance
{
    distance(mapnik::geometry::point<double> const& pt)
        : pt_(pt) {}

    double operator() (mapnik::geometry::geometry_empty const&) const
    {
        return 0;
    }
    double operator() (mapnik::geometry::geometry_collection<double> const&) const
    {
        return 0;
    }
    template <typename T>
    double operator() (T const& geom) const
    {
        return boost::geometry::distance(pt_, geom);
    }
    mapnik::geometry::point<double> pt_;
};


struct distance_info
{
    distance_info(mapnik::geometry::point<double> const& pt)
        : pt_(pt) {}

    double operator() (mapnik::geometry::geometry_empty const&) const
    {
        return 0;
    }
    double operator() (mapnik::geometry::geometry_collection<double> const&) const
    {
        return 0;
    }

    template <typename T>
    double operator() (T const& geom) const
    {
        return boost::geometry::distance(pt_, geom);
    }
    mapnik::geometry::point<double> pt_;
};


}

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
        //std::cerr << feature->envelope() << std::endl;
        //std::string wkt;
        //mapnik::util::to_wkt(wkt, feature->get_geometry());
        //std::cerr << wkt << std::endl;
        //std::cerr << feature->get(0) << std::endl;
        std::cerr << "   Comparable distance:"
                  << mapnik::util::apply_visitor(comparable_distance(pt), feature->get_geometry())
                  << std::endl;
        std::cerr << "   Distance:"
                  << mapnik::util::apply_visitor(distance_info(pt), feature->get_geometry())
                  << std::endl;

        auto const& geom = feature->get_geometry();
        if (geom.is<mapnik::geometry::polygon<double>>())
        {
            auto const& poly = geom.get<mapnik::geometry::polygon<double>>();
            mapper.add(poly);
            mapper.map(poly, "fill-opacity:0.3;fill:rgb(51,51,153);stroke:rgb(51,51,153);stroke-width:2");

            using result_type = boost::geometry::distance_info_result<mapnik::geometry::point<double>>;

            if (!poly.empty())
            {
                for (auto const& ring : poly)
                {
                    result_type info;
                    boost::geometry::distance_info(pt ,reinterpret_cast<mapnik::geometry::line_string<double> const&>(ring), info);
                    auto projected1 = info.projected_point1;
                    std::string wkt;
                    mapnik::util::to_wkt(wkt, projected1);
                    std::cerr << "projected point:" << wkt
                              << " real_distance=" << info.real_distance
                              << " fraction:" << info.fraction1
                              << " on_segment:" << info.on_segment
                              << " projected_distance:" << info.projected_distance1
                              << " source-index:" << info.seg_id1.source_index
                              << " segment-index:" << info.seg_id1.segment_index
                              << " piece-index:" << info.seg_id1.piece_index << std::endl;

                    std::cerr << "boost::distance=" << boost::geometry::distance(pt, ring) << std::endl;
                    mapper.add(projected1);
                    mapper.map(projected1,"fill-opacity:0.5;fill:green;stroke:green;stroke-width:2");
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
