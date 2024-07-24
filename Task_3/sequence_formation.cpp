#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/program_options.hpp>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <queue>
#include <stack>
#include <unordered_set>
#include <set>

namespace pt = boost::property_tree;
namespace po = boost::program_options;

auto json_to_machine(const std::string& json_path) {

    pt::ptree machine;
    pt::read_json(json_path, machine);
    return machine;
}

auto sequences_to_file(const std::string& output_file, const std::vector<std::vector<std::string>>& sequences) {

    std::ofstream file(output_file);
    if (!file.is_open()) {
        std::cerr << "Error opening output file!!!" << std::endl;
        return 2;
    }

    for (const auto& seq : sequences) {
        for (size_t i = 0; i < seq.size(); ++i) {
            file << seq[i];
            if (i < seq.size() - 1) {
                file << ",";
            }
        }
        file << "\n";
    }
    return 0;
}

auto get_all_states(const pt::ptree& machine) {
    std::set<std::string> states;

    const auto& transitions = machine.get_child("transitions");
    for (const auto& state : transitions) {
        states.insert(state.first);
    }

    for (const auto& transition : transitions) {
        const auto& transition_data = transition.second;
        for (const auto& sub_trans_data : transition_data) {
            const auto& next_state = sub_trans_data.second;
            states.insert(next_state.get<std::string>("state"));
        }
    }

    return states;
}

auto get_next_states(const pt::ptree& machine, const std::string& state) {
    std::vector<std::pair<std::string, std::string>> next_states;

    auto state_opt = machine.get_child_optional(state);

    if (!state_opt) {
        return next_states;
    }

    auto& state_node = *state_opt;
    for (const auto& transition : state_node) {
        std::string input_symb = transition.first;
        std::string next_state = transition.second.get<std::string>("state");

        next_states.push_back({input_symb, next_state});
    }

    return next_states;
}

void generate_state_sequences(const pt::ptree& machine, std::vector<std::vector<std::string>>& sequences) {
    auto initial_state = machine.get<std::string>("initial_state");

    // Используем get_child_optional для безопасного доступа к узлу
    auto initialTransitions = machine.get_child_optional("transitions." + initial_state);

    std::unordered_set<std::string> states;
    std::stack<std::pair<std::string, std::vector<std::string>>> stack;
    stack.push({initial_state, {}});

    while (!stack.empty()) {
        auto [currentState, currentSequence] = stack.top();
        stack.pop();

        if (states.find(currentState) != states.end()) {
            continue;
        }

        states.insert(currentState);
        sequences.push_back(currentSequence);

        auto transitions = machine.get_child_optional("transitions." + currentState);

        if (!transitions) {
            continue;
        }

        for (const auto& transition : *transitions) {
            const auto& input = transition.first;
            const auto& transitionDetails = transition.second;
            std::string nextState = transitionDetails.get<std::string>("state");

            std::vector<std::string> newSequence = currentSequence;
            newSequence.push_back(input);
            stack.push({nextState, newSequence});
        }
    }
}

/*
auto generate_state_sequences(const pt::ptree& machine, std::vector<std::vector<std::string>>& sequences) {
    // контейнер для хранения покрывающих этот автомат наборов последовательностей
    // при записи в итог, нужно будет выбрать элемент, состоящий из наименьшего числа последовательностей
    std::vector<std::vector<std::vector<std::string>>> covering_sequences;

    auto initial_state = machine.get<std::string>("initial_state");

    // теперь нужно получить множество всех присутсвующих в автомате состояний
    auto all_states = get_all_states(machine);

    // множество посещенных состояний
    std::set<std::string> visited_states;

    std::vector<std::string> current_seq;
    std::stack<std::string> depths;

    auto next_states_for_initial_state = get_next_states(machine, initial_state);
    for (size_t i = 0; i < next_states_for_initial_state.size(); i++) {
        auto next_q = get_next_states(machine, next_states_for_initial_state[i].second);
        auto next_symb_in = get_next_states(machine, next_states_for_initial_state[i].first);

        auto it_visited = std::find(visited_states.begin(), visited_states.end(), next_q);
        if (it_visited != visited_states.end())
        {

        }

    }


    для каждого состояния, нужно найти присоединенные исходящими из него переходами состояния, у которых 0 исходящих переходов - они будут "тупиками"
    т.к. из этих "тупиков" нельзя выйти, то кол-во покрывающих последовательностей будет точно не меньше кол-ва этих тупиков

}
*/

auto generate_transition_sequences(const pt::ptree& machine, std::vector<std::vector<std::string>>& sequences) {
}

auto generate_path_sequences(const pt::ptree& machine, int path_len, std::vector<std::vector<std::string>>& sequences) {
}

int main(int argc, char* argv[]) {
    try {

        std::string mode;
        unsigned int path_len;
        std::string input_file;
        std::string output_file;

        po::options_description desc("Allowed options");
        desc.add_options()("help", "produce help message")("mode", po::value<std::string>(&mode)->required(), "working mode (states/transitions/paths)")("path-len", po::value<unsigned int>(&path_len), "length of the path in paths mode")("in", po::value<std::string>(&input_file)->required(), "input JSON file path")("out", po::value<std::string>(&output_file)->required(), "output file path");

        po::positional_options_description p;
        p.add("input-file", -1);

        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);

        if (vm.count("help")) {
            std::cout << desc << "\n";
            return 0;
        }

        po::notify(vm);

        pt::ptree readed_machine = json_to_machine(input_file);

        std::vector<std::vector<std::string>> sequences;
        if (mode == "states") {

            generate_state_sequences(readed_machine, sequences);

        } else if (mode == "transitions") {

            generate_transition_sequences(readed_machine, sequences);

        } else if (mode == "paths") {

            if (path_len <= 0) {

                throw std::invalid_argument("Path length must be positive in paths mode");
            }
            generate_path_sequences(readed_machine, path_len, sequences);

        } else {
            throw std::invalid_argument("Invalid mode: " + mode + ". There're only 3 modes: states/transitions/paths");
        }
        sequences_to_file(output_file, sequences);

    } catch (const po::error& e) {
        std::cerr << "Command line error: " << e.what() << "\n";
        return 2;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 2;
    }
}