#ifndef UTILITY_FUNCTIONS_HPP
#define UTILITY_FUNCTIONS_HPP

#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/program_options.hpp>
#include <unordered_set>
#include <unordered_map>
#include <functional>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <queue>
#include <stack>
#include <list>
#include <set>

namespace pt = boost::property_tree;
namespace po = boost::program_options;

// общие функции
boost::property_tree::ptree json_to_machine(const std::string& json_path);
int sequences_to_file(const std::string& output_file, const std::vector<std::vector<std::string>>& sequences);

// для режима states
bool length_comparator_greater(const std::vector<std::string>& a, const std::vector<std::string>& b);
std::vector<std::vector<std::string>> remove_pyramidal_subduplicates(const std::vector<std::vector<std::string>>& sequences);

// для режима transitions
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

namespace std {
template <>
struct hash<Transition> {
    size_t operator()(const Transition& t) const {
        return hash<string>()(t.current_state) ^ hash<string>()(t.next_state) ^ hash<string>()(t.input_symbol) ^ hash<string>()(t.output_symbol);
    }
};
} // namespace std

std::vector<Transition> get_all_transitions(const pt::ptree& machine);
std::unordered_map<std::string, std::vector<Transition>> group_transitions_by_state(const std::vector<Transition>& transitions);

std::vector<Transition> find_single_branched_transitions(const std::unordered_map<std::string, std::vector<Transition>>& grouped_transitions);
std::unordered_map<std::string, std::vector<Transition>> find_multi_branched_transitions(const std::unordered_map<std::string, std::vector<Transition>>& grouped_transitions);

std::unordered_map<std::string, std::vector<Transition>> filter_unique_next_state(const std::unordered_map<std::string, std::vector<Transition>>& multi_branched);

std::list<Transition>* find_list_starting_with(std::vector<std::list<Transition>>& lists, const std::string& start_state);
std::list<Transition>* find_list_ending_with(std::vector<std::list<Transition>>& lists, const std::string& end_state);

std::vector<std::list<Transition>> find_linked_chains_for_single_branched(const std::vector<Transition>& single_transitions);

std::vector<Transition> find_self_loops(const std::vector<Transition>& transitions);
std::vector<std::list<Transition>> find_and_group_self_loops(const std::vector<Transition>& transitions);
std::vector<std::list<Transition>> merge_loops_and_singles(const std::vector<Transition>& singles, const std::vector<std::list<Transition>>& self_loops_chains);

std::vector<Transition> multies_to_singles(const std::unordered_map<std::string, std::vector<Transition>>& multies);

std::vector<std::list<Transition>> combine_multies_with_singles(std::unordered_map<std::string, std::vector<Transition>> multi_branched, std::vector<std::list<Transition>>& single_chains, std::string initial_state);
std::vector<std::list<Transition>> combine_singles_with_multies(std::vector<std::list<Transition>>& single_chains, std::unordered_map<std::string, std::vector<Transition>> multi_branched);

bool has_non_empty_vectors(const std::unordered_map<std::string, std::vector<Transition>>& map);
bool all_singles_used(const std::vector<std::list<Transition>>& chains_of_singles, const std::unordered_set<std::list<Transition>, Transition::ListHash>& used_single_chains);
bool all_multies_used(const std::unordered_map<std::string, std::vector<Transition>>& multi_branched, const std::unordered_set<std::string>& used_multi_branches);

std::vector<std::list<Transition>> connect_everything(std::unordered_map<std::string, std::vector<Transition>>& multi_branched, std::vector<std::list<Transition>>& chains_of_singles, const std::string& initial_state);

bool is_sublist(const std::list<Transition>& sub, const std::list<Transition>& full);
void remove_sublists(std::vector<std::list<Transition>>& sequences);

bool all_transitions_present(const std::vector<Transition>& transitions, const std::vector<std::list<Transition>>& result_sequences);
std::vector<std::list<Transition>> filter_check_present(const std::vector<std::list<Transition>>& unfiltered_sequences, const std::vector<Transition>& all_transitions);

// для режима paths
void remove_non_unique_substrings(std::vector<std::vector<std::string>>& sequences);
int find_max_path_len(const pt::ptree& transitions, const std::string& current_state, std::unordered_map<std::string, int>& memo, int input_length, std::unordered_set<std::string>& visited);

#endif