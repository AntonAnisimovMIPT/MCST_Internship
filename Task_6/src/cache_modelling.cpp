#include "functions.hpp"

int main(int argc, char* argv[]) {
    try {

        po::options_description desc("Allowed options");
        desc.add_options()("help", "produce help message")("config", po::value<std::string>(), "JSON file with cache description")("input", po::value<std::string>(), "File with operations");

        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);

        if (vm.count("help")) {
            std::cout << desc << std::endl;
            return 0;
        }

        if (!vm.count("config") || !vm.count("input")) {
            std::cerr << "Both config and input files must be specified." << std::endl;
            return 1;
        }

        auto config_file = vm["config"].as<std::string>();
        auto input_file = vm["input"].as<std::string>();

        pt::ptree root;
        pt::read_json(config_file, root);

        auto line_size = root.get<int>("cache_descr.line_size");
        auto associativity = root.get<int>("cache_descr.associativity");
        auto cache_size = root.get<int>("cache_descr.cache_size");

        Cache cache(cache_size, line_size, associativity);

        std::ifstream operations_file(input_file);
        if (!operations_file) {
            std::cerr << "Could not open input file: " << input_file << std::endl;
            return 1;
        }

        std::string line;
        while (std::getline(operations_file, line)) {
            std::istringstream iss(line);
            char op;
            std::uint32_t addr;
            std::uint32_t data;
            iss >> op >> std::hex >> addr >> std::hex >> data;

            if (op == 'R') {
                cache.read(addr, data);
            } else if (op == 'W') {
                cache.write(addr, data);
            } else {
                std::cerr << "Unknown operation: " << op << std::endl;
                return 1;
            }
        }

        return 0;

    } catch (const po::error& e) {
        std::cerr << "Command line error: " << e.what() << "\n";
        return 2;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 2;
    }
}
