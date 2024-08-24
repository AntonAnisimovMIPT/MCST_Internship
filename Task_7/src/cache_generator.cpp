#include "functions.hpp"
#include <boost/program_options.hpp>

namespace po = boost::program_options;

int main(int argc, char* argv[]) {
    try {
        std::string config_file;
        std::string output_file;
        unsigned int num_operations;
        double read_prob;
        unsigned int seed;

        po::options_description desc("Allowed options");
        desc.add_options()("help", "produce help message")("config", po::value<std::string>(&config_file)->required(), "JSON file with cache description")("seed", po::value<unsigned int>(&seed)->default_value(42), "Random seed")("num_operations", po::value<unsigned int>(&num_operations)->default_value(1000), "Number of operations to generate")("read_prob", po::value<double>(&read_prob)->default_value(0.5), "Probability of read operation")("output", po::value<std::string>(&output_file)->required(), "Output file for generated operations");

        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);

        if (vm.count("help")) {
            std::cout << desc << std::endl;
            return 0;
        }

        po::notify(vm);

        Generator generator(config_file, seed);
        generator.generate_tests(output_file, num_operations, read_prob);

    } catch (const po::error& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}