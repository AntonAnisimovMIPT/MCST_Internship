#include "utility_functions.hpp"

auto generate_state_sequences(const pt::ptree& machine, std::vector<std::vector<std::string>>& sequences) {

    auto initial_state = machine.get<std::string>("initial_state");
    auto initial_transitions = machine.get_child_optional("transitions");

    std::unordered_set<std::string> states;
    std::stack<std::pair<std::string, std::vector<std::string>>> stack;
    stack.push({initial_state, {}});

    while (!stack.empty()) {
        auto [current_state, current_sequence] = stack.top();
        stack.pop();

        if (states.find(current_state) != states.end()) {
            continue;
        }

        states.insert(current_state);
        sequences.push_back(current_sequence);

        auto transitions = machine.get_child_optional("transitions." + current_state);

        if (!transitions) {
            continue;
        }

        for (const auto& transition : *transitions) {
            const auto& input = transition.first;
            const auto& transition_data = transition.second;
            std::string next_state = transition_data.get<std::string>("state");

            std::vector<std::string> new_sequence = current_sequence;
            new_sequence.push_back(input);
            stack.push({next_state, new_sequence});
        }
    }
}

auto generate_transition_sequences(const pt::ptree& machine, std::vector<std::vector<std::string>>& sequences) {

    // Этап #1: Подготовка цепочек
    auto transitions = get_all_transitions(machine);
    auto grouped_transitions = group_transitions_by_state(transitions);

    auto singles = find_single_branched_transitions(grouped_transitions);
    auto self_loops_chains = find_and_group_self_loops(transitions);
    auto loops_and_singles = merge_loops_and_singles(singles, self_loops_chains);

    // Этап #2: Связывание цепочек
    auto multi_branched = find_multi_branched_transitions(grouped_transitions);
    auto initial_state = machine.get<std::string>("initial_state");
    auto result_sequences = connect_everything(multi_branched, loops_and_singles, initial_state);

    // Этап #3: фильтрация последовательностей и проверка покрытия всех состояний
    remove_sublists(result_sequences);
    auto final_sequences = filter_check_present(result_sequences, transitions);

    // Этап #4: построение итоговых последовательностей выходных символов
    for (const auto& transition_list : final_sequences) {
        std::vector<std::string> string_vector;
        for (const auto& transition : transition_list) {
            std::stringstream ss;
            ss << transition.input_symbol;
            string_vector.push_back(ss.str());
        }
        sequences.push_back(string_vector);
    }

    return 0;
}

/*
auto generate_path_sequences(const pt::ptree& machine, int path_len, std::vector<std::vector<std::string>>& sequences) {
    auto initial_state = machine.get<std::string>("initial_state");
    auto transitions = machine.get_child("transitions");

    std::function<int(const std::string&, int)> find_max_path_len = [&](const std::string& current_state, int current_len) {
        int max_len = current_len;

        auto transitions_opt = transitions.get_child_optional(current_state);
        if (!transitions_opt) return max_len;

        for (const auto& transition : transitions_opt.get()) {
            const auto& transition_data = transition.second;
            std::string next_state = transition_data.get<std::string>("state");

            int len = find_max_path_len(next_state, current_len + 1);
            max_len = std::max(max_len, len);
        }

        return max_len;
    };

    int max_path_len = find_max_path_len(initial_state, 0);
    path_len = std::min(path_len, max_path_len);

    std::function<void(const std::string&, std::vector<std::string>&, int)> dfs =
        [&](const std::string& current_state, std::vector<std::string>& current_sequence, int remaining_len) {
            if (remaining_len == 0) {
                sequences.push_back(current_sequence);
                return;
            }

            auto transitions_opt = transitions.get_child_optional(current_state);
            if (!transitions_opt) return;

            for (const auto& transition : transitions_opt.get()) {
                const std::string& input = transition.first;
                const auto& transition_data = transition.second;
                std::string next_state = transition_data.get<std::string>("state");

                current_sequence.push_back(input);
                dfs(next_state, current_sequence, remaining_len - 1);
                current_sequence.pop_back();
            }
        };

    std::vector<std::string> initial_sequence;
    dfs(initial_state, initial_sequence, path_len);
}
*/
auto generate_path_sequences(const pt::ptree& machine, int path_len, std::vector<std::vector<std::string>>& sequences) {
    auto initial_state = machine.get<std::string>("initial_state");
    auto transitions = machine.get_child("transitions");

    std::unordered_map<std::string, int> memo;
    std::unordered_set<std::string> visited;
    auto max_path_len = find_max_path_len(transitions, initial_state, memo, path_len, visited);
    auto real_path_len = std::min(max_path_len, path_len);

    std::function<void(const std::string&, std::vector<std::string>&, int)> dfs =
        [&](const std::string& current_state, std::vector<std::string>& current_sequence, int remaining_len) {
            if (remaining_len == 0) {
                sequences.push_back(current_sequence);
                return;
            }

            auto transitions_opt = transitions.get_child_optional(current_state);
            if (!transitions_opt) return;

            for (const auto& transition : transitions_opt.get()) {
                const std::string& input = transition.first;
                const auto& transition_data = transition.second;
                std::string next_state = transition_data.get<std::string>("state");

                current_sequence.push_back(input);
                dfs(next_state, current_sequence, remaining_len - 1);
                current_sequence.pop_back();
            }
        };

    std::vector<std::string> initial_sequence;
    dfs(initial_state, initial_sequence, real_path_len);
}

int main(int argc, char* argv[]) {
    try {

        std::string mode;
        unsigned int path_len;
        std::string input_file;
        std::string output_file;

        po::options_description desc("Allowed options");
        desc.add_options()("help", "produce help message")("mode", po::value<std::string>(&mode)->required(), "working mode (states/transitions/paths)")("path-len", po::value<unsigned int>(&path_len), "length of the path in paths mode")("input-file", po::value<std::string>(&input_file)->required(), "input JSON file path")("out", po::value<std::string>(&output_file)->required(), "output file path");

        po::positional_options_description p;
        p.add("input-file", 1);

        po::variables_map vm;
        po::store(po::command_line_parser(argc, argv).options(desc).positional(p).run(), vm);

        if (vm.count("help")) {
            std::cout << desc << "\n";
            return 0;
        }

        po::notify(vm);

        pt::ptree readed_machine = json_to_machine(input_file);

        std::vector<std::vector<std::string>> sequences;
        if (mode == "states") {

            generate_state_sequences(readed_machine, sequences);
            sequences = remove_pyramidal_subduplicates(sequences);

        } else if (mode == "transitions") {

            generate_transition_sequences(readed_machine, sequences);

        } else if (mode == "paths") {

            if (path_len <= 0) {

                throw std::invalid_argument("Path length must be positive in paths mode");
            }
            generate_path_sequences(readed_machine, path_len, sequences);
            remove_non_unique_substrings(sequences);

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