#ifndef FUNCTIONS_HPP
#define FUNCTIONS_HPP

#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/program_options.hpp>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <iostream>
#include <iterator>
#include <random>
#include <vector>
#include <list>

struct Transition {
    std::string current_state;
    std::string next_state;
    std::string input_symbol;
    std::string output_symbol;

    bool operator==(const Transition& other) const {
        return current_state == other.current_state &&
            next_state == other.next_state &&
            input_symbol == other.input_symbol &&
            output_symbol == other.output_symbol;
    }

    struct Hash {
        std::size_t operator()(const Transition& t) const {
            std::size_t h1 = std::hash<std::string>{}(t.current_state);
            std::size_t h2 = std::hash<std::string>{}(t.next_state);
            std::size_t h3 = std::hash<std::string>{}(t.input_symbol);
            std::size_t h4 = std::hash<std::string>{}(t.output_symbol);
            return h1 ^ (h2 << 1) ^ (h3 << 2) ^ (h4 << 3);
        }
    };

    struct ListHash {
        std::size_t operator()(const std::list<Transition>& list) const {
            std::size_t seed = list.size();
            for (const auto& transition : list) {
                seed ^= Transition::Hash{}(transition) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            }
            return seed;
        }
    };
};

struct VectorHash {
    std::size_t operator()(const std::vector<std::string>& v) const {
        std::size_t seed = v.size();
        for (const auto& str : v) {
            seed ^= std::hash<std::string>{}(str) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        }
        return seed;
    }
};

namespace std {
template <>
struct hash<Transition> {
    size_t operator()(const Transition& t) const {
        return hash<string>()(t.current_state) ^ hash<string>()(t.next_state) ^ hash<string>()(t.input_symbol) ^ hash<string>()(t.output_symbol);
    }
};
} // namespace std

namespace pt = boost::property_tree;
namespace po = boost::program_options;

pt::ptree json_to_machine(const std::string& json_path);
std::vector<std::vector<std::string>> read_sequences(const std::string& sequences_file);
std::unordered_set<std::string> get_all_states(const pt::ptree& machine);
std::unordered_set<Transition> get_all_transitions(const pt::ptree& machine);

using Path = std::vector<Transition>;

struct PathHash {
    std::size_t operator()(const Path& path) const {
        std::size_t seed = path.size();
        for (const auto& transition : path) {
            seed ^= Transition::Hash{}(transition) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        }
        return seed;
    }
};
std::unordered_set<Path, PathHash> get_all_paths(const pt::ptree& machine, int path_len);

bool is_valid_path(const pt::ptree& machine, const std::vector<std::string>& path, const std::string& initial_state);
int find_max_path_len(const pt::ptree& transitions, const std::string& current_state, std::unordered_map<std::string, int>& memo, int input_length, std::unordered_set<std::string>& visited);
bool verify_etalon_in_sequences(const std::vector<std::vector<std::string>>& etalon, const std::vector<std::vector<std::string>>& sequences);
std::vector<Transition> find_transitions_from_state(const pt::ptree& machine, const std::string& state);
#endif