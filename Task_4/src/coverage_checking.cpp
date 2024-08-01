#include "functions.hpp"

auto check_coverage_states(const pt::ptree& machine, const std::vector<std::vector<std::string>>& sequences) {

    auto all_states = get_all_states(machine);

    auto initial_state = machine.get<std::string>("initial_state");
    std::unordered_set<std::string> visited_states;
    visited_states.insert(initial_state); // т.к. пустые автоматы не рассматриваем

    for (const auto& sequence : sequences) {
        std::string current_state = initial_state;
        for (const auto& input : sequence) {
            try {

                auto state_transitions = machine.get_child("transitions").get_child(current_state);
                auto next_state = state_transitions.get<std::string>(input + ".state");

                visited_states.insert(next_state);
                current_state = next_state;
            } catch (const pt::ptree_error& e) {
                std::string msg = "Transition not found for state: " + current_state + " with input: " + input + "\n";
                throw std::runtime_error(msg);
            }
        }
    }

    // вывод всех непосещенных состояний, если они есть
    std::ostringstream missing_states_msg;
    auto has_missing_states = false;
    for (const auto& state : all_states) {
        if (visited_states.find(state) == visited_states.end()) {
            if (has_missing_states) {
                missing_states_msg << ", ";
            }
            missing_states_msg << state;
            has_missing_states = true;
        }
    }

    if (has_missing_states) {
        std::string msg = "States not covered: " + missing_states_msg.str() + "\n";
        throw std::runtime_error(msg);
    }

    return 0;
}

auto check_coverage_transitions(const pt::ptree& machine, const std::vector<std::vector<std::string>>& sequences) {

    auto all_transitions = get_all_transitions(machine);

    std::unordered_set<Transition, Transition::Hash> visited_transitions;
    std::string initial_state = machine.get<std::string>("initial_state");

    for (const auto& sequence : sequences) {
        std::string current_state = initial_state;
        for (const auto& input : sequence) {
            try {
                auto transitions = machine.get_child("transitions").get_child(current_state);
                std::string next_state = transitions.get<std::string>(input + ".state");
                std::string output = transitions.get<std::string>(input + ".output");
                visited_transitions.insert({current_state, next_state, input, output});
                current_state = next_state;
            } catch (const pt::ptree_error& e) {
                std::string msg = "Error processing transition from state " + current_state + " with input: " + input + "\n";
                throw std::runtime_error(msg);
            }
        }
    }

    // вывод всех непосещенных переходов, если они есть
    std::ostringstream missing_transitions_msg;
    auto has_missing_transitions = false;
    for (const auto& transition : all_transitions) {
        if (visited_transitions.find(transition) == visited_transitions.end()) {
            if (has_missing_transitions) {
                missing_transitions_msg << "\n";
            }
            missing_transitions_msg << "[" << transition.current_state << " -> "
                                    << transition.next_state << " (input: "
                                    << transition.input_symbol << ", output: "
                                    << transition.output_symbol << ")]";
            has_missing_transitions = true;
        }
    }

    if (has_missing_transitions) {
        std::string msg = "Transitions not covered: " + missing_transitions_msg.str() + "\n";
        throw std::runtime_error(msg);
    }

    return 0;
}

auto check_coverage_paths(const pt::ptree& machine, const std::vector<std::vector<std::string>>& sequences, unsigned int path_len) {
    auto initial_state = machine.get<std::string>("initial_state");

    for (const auto& sequence : sequences) {
        if (sequence.size() > path_len) {
            std::string msg_dt;
            for (const auto& input : sequence) {
                msg_dt += input;
                msg_dt += ",";
            }
            auto msg = "Sequence length exceeds path length: " + msg_dt;
            throw std::runtime_error(msg);
        }

        if (!is_valid_path(machine, sequence, initial_state)) {
            std::string msg_dt;
            for (const auto& input : sequence) {
                msg_dt += input;
                msg_dt += ",";
            }
            auto msg = "Path is invalid: " + msg_dt;
            throw std::runtime_error(msg);
        }
    }
    return 0;
}

int main(int argc, char* argv[]) {
    try {

        std::string mode;
        unsigned int path_len;
        std::string json_description;
        std::string sequences_file;

        po::options_description desc("Allowed options");
        desc.add_options()("help", "produce help message")("mode", po::value<std::string>(&mode)->required(), "working mode (states/transitions/paths)")("path-len", po::value<unsigned int>(&path_len), "length of the path in paths mode")("json-description", po::value<std::string>(&json_description)->required(), "JSON description file path")("seq", po::value<std::string>(&sequences_file)->required(), "checked sequence file path");

        po::positional_options_description p;
        p.add("json-description", 1);

        po::variables_map vm;
        po::store(po::command_line_parser(argc, argv).options(desc).positional(p).run(), vm);

        if (vm.count("help")) {
            std::cout << desc << "\n";
            return 0;
        }

        po::notify(vm);

        auto readed_machine = json_to_machine(json_description);
        auto sequences = read_sequences(sequences_file);

        if (mode == "states") {

            check_coverage_states(readed_machine, sequences);

        } else if (mode == "transitions") {

            check_coverage_transitions(readed_machine, sequences);

        } else if (mode == "paths") {

            if (path_len <= 0) {

                throw std::invalid_argument("Path length must be positive in paths mode");
            }
            check_coverage_paths(readed_machine, sequences, path_len);

        } else {
            throw std::invalid_argument("Invalid mode: " + mode + ". There're only 3 modes: states/transitions/paths");
        }

    } catch (const po::error& e) {
        std::cerr << "Command line error: " << e.what() << "\n";
        return 2;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 2;
    }
}