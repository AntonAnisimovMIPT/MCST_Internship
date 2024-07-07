#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/program_options.hpp>
#include <iostream>
#include <fstream>
#include <locale>
#include <string>

namespace pt = boost::property_tree;
namespace po = boost::program_options;

void convert(const std::string &json_path, const std::string &DOT_path) {
    pt::ptree tree;
    pt::read_json(json_path, tree);

    std::ofstream DOT_file(DOT_path);
    if (!DOT_file.is_open()) {
        std::cerr << "Error opening .DOT file!!! " << std::endl;
        return;
    }

    DOT_file << "digraph {\n";

    std::string initial_state = tree.get<std::string>("initial_state");
    DOT_file << "    start [style=invis];\n" << "    start -> " << initial_state << " ;\n";

    for (const auto &state : tree.get_child("transitions")) {
        std::string state_name = state.first;

        for (const auto &transition : state.second) {
            std::string input_symbol = transition.first;
            std::string output_symbol = transition.second.get<std::string>("output");
            std::string next_state = transition.second.get<std::string>("state");
            
            DOT_file << "    " << state_name << " -> " << next_state 
                     << " [label=\"" << input_symbol << "/" << output_symbol << "\"];\n";
        }
    }

    DOT_file << "}\n";
    DOT_file.close();
}

int main(int argc, char* argv[]) {

    std::string inputJsonPath = argv[1];
    std::string outputDotPath = argv[2];

    convert(inputJsonPath, outputDotPath);

    return 0;
}