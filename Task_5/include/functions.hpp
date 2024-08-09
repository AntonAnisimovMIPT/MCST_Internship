#ifndef FUNCTIONS_HPP
#define FUNCTIONS_HPP

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <unordered_map>

namespace pt = boost::property_tree;

enum class State;
enum class Output;

struct CacheLine {
    std::vector<int> data_block;
    int tag;
    bool valid;

    CacheLine(int size)
        : data_block(size), valid(false) {}
};

struct Transition {
    std::string input;
    std::string output;
    std::string current_state;
    std::string next_state;
};

void save_json_to_file(const pt::ptree& json, const std::string& filename);
std::string vector_to_string(const std::vector<int>& vec);

#endif