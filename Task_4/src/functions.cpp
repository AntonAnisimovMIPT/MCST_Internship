#include "functions.hpp"

pt::ptree json_to_machine(const std::string& json_path) {

    pt::ptree machine;
    pt::read_json(json_path, machine);
    return machine;
}

std::vector<std::vector<std::string>> read_sequences(const std::string& sequences_file) {
    std::ifstream infile(sequences_file);
    std::vector<std::vector<std::string>> sequences;
    std::string line;

    if (!infile.is_open()) {
        throw std::runtime_error("Failed to open file: " + sequences_file);
    }

    while (std::getline(infile, line)) {
        std::vector<std::string> sequence;
        std::string item;
        std::istringstream iss(line);

        while (std::getline(iss, item, ',')) {
            sequence.push_back(item);
        }

        if (!sequence.empty()) {
            sequences.push_back(sequence);
        }
    }

    return sequences;
}

std::unordered_set<std::string> get_all_states(const pt::ptree& machine) {
    std::unordered_set<std::string> all_states;

    const auto& transitions = machine.get_child("transitions");
    for (const auto& item : transitions) {
        const auto& state_transitions = item.second;

        for (const auto& transition : state_transitions) {
            const auto& transition_data = transition.second;
            auto state_opt = transition_data.get_optional<std::string>("state");
            if (state_opt) {
                all_states.insert(state_opt.value());
            }
        }
    }

    return all_states;
}

std::unordered_set<Transition> get_all_transitions(const pt::ptree& machine) {
    std::unordered_set<Transition> transitions;

    for (const auto& state_pair : machine.get_child("transitions")) {
        for (const auto& symbol_pair : state_pair.second) {
            Transition transition;
            transition.current_state = state_pair.first;
            transition.next_state = symbol_pair.second.get<std::string>("state");
            transition.input_symbol = symbol_pair.first;
            transition.output_symbol = symbol_pair.second.get<std::string>("output");
            transitions.insert(transition);
        }
    }

    return transitions;
}

bool is_valid_path(const pt::ptree& machine, const std::vector<std::string>& path, const std::string& initial_state) {
    std::string current_state = initial_state;

    for (const auto& input : path) {
        try {
            auto transitions = machine.get_child("transitions").get_child(current_state);
            auto next_state = transitions.get<std::string>(input + ".state");
            current_state = next_state;
        } catch (const pt::ptree_error& e) {
            return false;
        }
    }
    return true;
}

int find_max_path_len(const pt::ptree& transitions, const std::string& current_state, std::unordered_map<std::string, int>& memo, int input_length, std::unordered_set<std::string>& visited) {
    if (visited.find(current_state) != visited.end()) {
        return input_length;
    }

    visited.insert(current_state);

    if (memo.find(current_state) != memo.end()) {
        visited.erase(current_state);
        return memo[current_state];
    }

    int max_len = 0;
    auto transitions_opt = transitions.get_child_optional(current_state);
    if (!transitions_opt) {
        visited.erase(current_state);
        return max_len;
    }

    for (const auto& transition : transitions_opt.get()) {
        if (!transition.second.get_optional<std::string>("state")) {
            continue;
        }

        std::string next_state = transition.second.get<std::string>("state");

        if (max_len == input_length) {
            return input_length;
        }

        int len = 1 + find_max_path_len(transitions, next_state, memo, input_length, visited);
        max_len = std::max(max_len, len);
    }

    memo[current_state] = max_len;
    visited.erase(current_state);
    return max_len;
}

bool verify_etalon_in_sequences(const std::vector<std::vector<std::string>>& etalon, const std::vector<std::vector<std::string>>& sequences) {
    for (const auto& etalon_vector : etalon) {
        auto it = std::find(sequences.begin(), sequences.end(), etalon_vector);

        if (it == sequences.end()) {
            return false;
        }
    }
    return true;
}

std::vector<Transition> find_transitions_from_state(const pt::ptree& machine, const std::string& state) {
    std::vector<Transition> transitions;
    auto transitions_node = machine.get_child("transitions");
    auto state_node_opt = transitions_node.get_child_optional(state);
    if (state_node_opt) {
        for (const auto& item : state_node_opt.get()) {
            Transition t;
            t.current_state = state;
            t.input_symbol = item.first;
            t.next_state = item.second.get<std::string>("state");
            t.output_symbol = item.second.get<std::string>("output");
            transitions.push_back(t);
        }
    }

    return transitions;
}