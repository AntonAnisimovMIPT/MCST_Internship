#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/program_options.hpp>
#include <iostream>
#include <fstream>
#include <string>

namespace pt = boost::property_tree;
namespace po = boost::program_options;

int convert(const std::string &json_path, const std::string &DOT_path) {
    pt::ptree tree;
    pt::read_json(json_path, tree);

    std::ofstream DOT_file(DOT_path);
    if (!DOT_file.is_open()) {
        std::cerr << "Error opening .DOT file!!! " << std::endl;
        return 2;
    }

    DOT_file << "digraph {\n";

    std::string initial_state = tree.get<std::string>("initial_state");
    DOT_file << "    " << initial_state << " [shape=doublecircle];\n";

    for (const auto &state : tree.get_child("transitions")) {
        std::string state_name = state.first;

        if (state.second.empty()) {
            std::cerr << "Warning: State '" << state_name << "' has empty transitions\n";
            return 1;
        }

        for (const auto &transition : state.second) {
            std::string input_symbol = transition.first;

            if (transition.second.empty()) {
                std::cerr << "Warning: Transition '" << input_symbol << "' in state '" << state_name << "' is empty\n";
                return 1;
            }
            std::string output_symbol = transition.second.get<std::string>("output");
            std::string next_state = transition.second.get<std::string>("state");

            if (input_symbol.empty() || output_symbol.empty() || next_state.empty() || state_name == "") {
                std::cerr << "Warning: Empty transition detected in state '" << state_name << std::endl;
                return 1;
            }
            
            DOT_file << "    " << state_name << " -> " << next_state 
                     << " [label=\"" << input_symbol << "/" << output_symbol << "\"];\n";
        }
    }

    DOT_file << "}\n";
    DOT_file.close();
    return 0;
}

int main(int argc, char* argv[]) {
    try {
        po::options_description desc("Allowed options");
        desc.add_options()
            ("help", "produce help message")
            ("input", po::value<std::string>(), "set input .json file")
            ("output", po::value<std::string>(), "set output .DOT file");

        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);

        if (vm.count("help")) {
            std::cout << desc << "\n";
            return 1;
        }

        if (!vm.count("input") || !vm.count("output")) {
            std::cerr << "Both input and output file paths must be specified.\n";
            std::cout << desc << "\n";
            return 1;
        }

        std::string input_path = vm["input"].as<std::string>();
        std::string output_path = vm["output"].as<std::string>();

        return convert(input_path, output_path);

    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}