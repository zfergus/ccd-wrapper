// Time the different CCD methods

#include <iostream>

#include <array>
#include <string>

#include <Eigen/Core>
#include <boost/filesystem.hpp>
#include <highfive/H5Easy.hpp>
#include <igl/Timer.h>

#include <ccd.hpp>

int main(int argc, char* argv[])
{
    std::stringstream usage;
    usage << "usage: " << argv[0] << " /path/to/data/ {vf|ee}";
    if (argc < 2) {
        std::cerr << usage.str() << std::endl
                  << "error: missing path to data" << std::endl;
        return 1;
    }
    if (argc < 3) {
        std::cerr << usage.str() << std::endl
                  << "error: missing type of collisions" << std::endl;
        return 2;
    } else if (strcmp(argv[2], "vf") != 0 && strcmp(argv[2], "ee") != 0) {
        std::cerr << usage.str() << std::endl
                  << "invalid type of collisions: " << argv[2] << std::endl;
        return 2;
    }

    std::string data_dir(argv[1]);

    bool is_edge_edge = strcmp(argv[2], "ee") == 0;

    std::cout << "Using " << (is_edge_edge ? "edge-edge" : "vertex-face")
              << " CCD" << std::endl;

    igl::Timer timer;

    int num_queries = 0;
    std::array<double, 5> timings = { { 0, 0, 0, 0, 0 } };
    std::array<int, 5> false_positives = { { 0, 0, 0, 0, 0 } };
    std::array<int, 5> false_negatives = { { 0, 0, 0, 0, 0 } };

    int i = 0;
    for (auto& entry : boost::filesystem::directory_iterator(data_dir)) {
        H5Easy::File file(entry.path().string());

        HighFive::Group root = file.getGroup("/");
        num_queries += root.getNumberObjects();

        const auto query_names = root.listObjectNames();
        for (const auto query_name : query_names) {
            Eigen::MatrixXd V = H5Easy::load<Eigen::MatrixXd>(file, query_name);
            std::array<bool, 5> results;

            // Time the methods
            for (int method = 0; method < 5; method++) {
                timer.start();
                if (is_edge_edge) {
                    results[method] = ccd::edgeEdgeCCD(
                        V.row(0), V.row(1), V.row(2), V.row(3), V.row(4),
                        V.row(5), V.row(6), V.row(7), ccd::CCDMethod(method));
                } else {
                    results[method] = ccd::vertexFaceCCD(
                        V.row(0), V.row(1), V.row(2), V.row(3), V.row(4),
                        V.row(5), V.row(6), V.row(7), ccd::CCDMethod(method));
                }
                timer.stop();
                timings[method] += timer.getElapsedTimeInMicroSec();
            }

            // Count the inaccuracies
            bool expected_result
                = results[int(ccd::CCDMethod::RATIONAL_ROOT_PARITY)];
            for (int method = 0; method < 5; method++) {
                if (results[method] != expected_result) {
                    if (results[method]) {
                        false_positives[method]++;
                    } else {
                        false_negatives[method]++;
                    }
                }
            }
            std::cout << i++ << "\r" << std::flush;
        }
    }

    std::cout << "# Queries: " << num_queries << std::endl;
    for (int method = 0; method < 5; method++) {
        std::cout << std::endl;
        std::cout << ccd::method_names[method] << ":" << std::endl;
        std::cout << "avg query time:    " << timings[method] / num_queries
                  << "μs" << std::endl;
        std::cout << "# of false positives: " << false_positives[method]
                  << std::endl;
        std::cout << "# of false negatives: " << false_negatives[method]
                  << std::endl;
    }
}
