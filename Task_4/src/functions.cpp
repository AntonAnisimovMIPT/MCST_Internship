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

std::unordered_set<Path, PathHash> get_all_paths(const pt::ptree& machine, int path_len) {
    std::unordered_set<Path, PathHash> paths;
    std::vector<std::pair<std::string, Path>> frontier;
    frontier.emplace_back(machine.get<std::string>("initial_state"), Path{});

    while (!frontier.empty()) {
        auto [current_state, current_path] = frontier.back();
        frontier.pop_back();

        if (current_path.size() == path_len) {
            paths.insert(current_path);
            continue;
        }

        const auto& state_transitions = machine.get_child("transitions").get_child(current_state);
        for (const auto& symbol_pair : state_transitions) {
            Transition transition;
            transition.current_state = current_state;
            transition.input_symbol = symbol_pair.first;
            transition.next_state = symbol_pair.second.get<std::string>("state");
            transition.output_symbol = symbol_pair.second.get<std::string>("output");

            Path new_path = current_path;
            new_path.push_back(transition);
            frontier.emplace_back(transition.next_state, new_path);
        }
    }

    return paths;
}
