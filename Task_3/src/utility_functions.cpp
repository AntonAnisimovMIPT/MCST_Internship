#include "utility_functions.hpp"

boost::property_tree::ptree json_to_machine(const std::string& json_path) {

    pt::ptree machine;
    pt::read_json(json_path, machine);
    return machine;
}

int sequences_to_file(const std::string& output_file, const std::vector<std::vector<std::string>>& sequences) {

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

bool length_comparator_greater(const std::vector<std::string>& a, const std::vector<std::string>& b) {
    return a.size() > b.size();
}

std::vector<std::vector<std::string>> remove_pyramidal_subduplicates(const std::vector<std::vector<std::string>>& sequences) {
    std::vector<std::vector<std::string>> result;
    std::set<std::vector<std::string>> unique_sequences;

    auto sorted_sequences = sequences;
    std::sort(sorted_sequences.begin(), sorted_sequences.end(), length_comparator_greater);

    // Проходим по отсортированным последовательностям и добавляем только уникальные длинные версии (т.е. непирамидальные поддубликатные)
    for (const auto& seq : sorted_sequences) {
        auto is_prefix = false;
        for (const auto& existing : unique_sequences) {
            if (std::mismatch(seq.begin(), seq.end(), existing.begin()).first == seq.end()) {
                is_prefix = true;
                break;
            }
        }
        if (!is_prefix) {
            unique_sequences.insert(seq);
        }
    }

    result.assign(unique_sequences.begin(), unique_sequences.end());
    return result;
}

std::vector<Transition> get_all_transitions(const pt::ptree& machine) {
    std::vector<Transition> transitions;

    for (const auto& state_pair : machine.get_child("transitions")) {
        for (const auto& symbol_pair : state_pair.second) {
            Transition transition;
            transition.current_state = state_pair.first;
            transition.next_state = symbol_pair.second.get<std::string>("state");
            transition.input_symbol = symbol_pair.first;
            transition.output_symbol = symbol_pair.second.get<std::string>("output");
            transitions.push_back(transition);
        }
    }

    return transitions;
}

std::unordered_map<std::string, std::vector<Transition>> group_transitions_by_state(const std::vector<Transition>& transitions) {
    std::unordered_map<std::string, std::vector<Transition>> grouped_transitions;

    for (const auto& transition : transitions) {

        auto it = grouped_transitions.find(transition.current_state);

        if (it == grouped_transitions.end()) {
            grouped_transitions[transition.current_state] = {transition};
        } else {

            it->second.push_back(transition);
        }
    }

    return grouped_transitions;
}

// внимание: одноразветвленность допускает, что у состояния могут быть self-loop переходы, но при этом должен быть только 1 обычный переход. При этом в итоговый результат идет обычный переход. Self-loop переходы будут находиться позднее.
std::vector<Transition> find_single_branched_transitions(const std::unordered_map<std::string, std::vector<Transition>>& grouped_transitions) {
    std::vector<Transition> result;

    for (const auto& element : grouped_transitions) {
        const auto& transitions = element.second;

        // Проверка наличия переходов, у которых next_state равен current_state
        auto number_self_referential = 0;
        Transition non_self_referential_transition;
        for (const auto& transition : transitions) {
            if (transition.current_state == transition.next_state) {
                number_self_referential++;
            } else {
                non_self_referential_transition = transition;
            }
        }

        // Если даже и есть самоссылающиеся переходы, но число обычных переходов = 1, то добавляем обычный. Самосссылающиеся переходы будем отдельно обрабатывать
        if (transitions.size() - number_self_referential == 1) {
            result.push_back(non_self_referential_transition);
        }
    }

    return result;
}

// внимание: в многоразветвленности не учитываются self-loops переходы, тут разветвленими считаются переходы, ведущие в отличные от иходного состояния
std::unordered_map<std::string, std::vector<Transition>> find_multi_branched_transitions(const std::unordered_map<std::string, std::vector<Transition>>& grouped_transitions) {
    std::unordered_map<std::string, std::vector<Transition>> result;

    for (const auto& [state, transitions] : grouped_transitions) {
        std::vector<Transition> non_self_transitions;

        for (const auto& transition : transitions) {
            if (transition.current_state != transition.next_state) {
                non_self_transitions.push_back(transition);
            }
        }

        if (non_self_transitions.size() > 1) {
            result[state] = non_self_transitions;
        }
    }

    return result;
}

std::unordered_map<std::string, std::vector<Transition>> filter_unique_next_state(const std::unordered_map<std::string, std::vector<Transition>>& multi_branched) {
    std::unordered_map<std::string, std::vector<Transition>> result;
    std::unordered_set<std::string> seen_next_states;

    for (const auto& [state, transitions] : multi_branched) {

        std::vector<Transition> filtered_transitions;

        for (const auto& transition : transitions) {
            if (seen_next_states.find(transition.next_state) == seen_next_states.end()) {
                filtered_transitions.push_back(transition);
                seen_next_states.insert(transition.next_state);
            }
        }

        if (!filtered_transitions.empty()) {
            result[state] = filtered_transitions;
        }
    }

    return result;
}

std::list<Transition>* find_list_starting_with(std::vector<std::list<Transition>>& lists, const std::string& start_state) {
    for (auto& list : lists) {
        if (!list.empty() && list.front().current_state == start_state) {
            return &list;
        }
    }
    return nullptr;
}

std::list<Transition>* find_list_ending_with(std::vector<std::list<Transition>>& lists, const std::string& end_state) {
    for (auto& list : lists) {
        if (!list.empty() && list.back().next_state == end_state) {
            return &list;
        }
    }
    return nullptr;
}

// внимание: данная функция применима только после find_single_branched_transitions, т.к. предполагается, что на первом этапе связывание происходит только для одноразветвленных
// также стоит обратить внимание, что она не работает с элементами вида q_i -> q_i, в дальнейшем эти элементы будут просто добавляться в данные цепочки
std::vector<std::list<Transition>> find_linked_chains_for_single_branched(const std::vector<Transition>& single_transitions) {

    std::vector<std::list<Transition>> linked_lists;
    std::vector<Transition> used_transitions;

    for (size_t i = 0; i < single_transitions.size(); i++) {

        auto it_i = std::find(used_transitions.begin(), used_transitions.end(), single_transitions[i]);
        if (it_i == used_transitions.end()) {

            // нужно понять, нельзя ли добавить данное звено к уже существующей цепочке
            auto is_started = find_list_starting_with(linked_lists, single_transitions[i].next_state);
            auto is_ended = find_list_ending_with(linked_lists, single_transitions[i].current_state);
            if (is_started) {
                is_started->push_front(single_transitions[i]);
                used_transitions.push_back(single_transitions[i]);
            } else if (is_ended) {
                is_ended->push_back(single_transitions[i]);
                used_transitions.push_back(single_transitions[i]);
            }
            // если к существующей нельзя добавить, то это будет уже новая цепочка, она возможно потом соеденится с другой
            else {
                linked_lists.push_back({single_transitions[i]});
                used_transitions.push_back(single_transitions[i]);
            }
        }
    }
    // Объединение цепочек, которые можно объединить
    for (size_t i = 0; i < linked_lists.size(); ++i) {
        for (size_t j = 0; j < linked_lists.size(); ++j) {
            if (i != j) {
                auto& list_i = linked_lists[i];
                auto& list_j = linked_lists[j];

                // Проверяем, можно ли объединить списки
                if (list_i.back().next_state == list_j.front().current_state) {
                    list_i.splice(list_i.end(), list_j);
                    linked_lists.erase(linked_lists.begin() + j);
                    --j;
                } else if (list_j.back().next_state == list_i.front().current_state) {
                    list_j.splice(list_j.end(), list_i);
                    linked_lists.erase(linked_lists.begin() + i);
                    --i;
                    break;
                }
            }
        }
    }

    return linked_lists;
}

// для нахождения элементов вида q_i -> q_i (self-loops переходы)
std::vector<Transition> find_self_loops(const std::vector<Transition>& transitions) {
    std::vector<Transition> self_loops;

    for (const auto& transition : transitions) {
        if (transition.current_state == transition.next_state) {
            self_loops.push_back(transition);
        }
    }

    return self_loops;
}

std::vector<std::list<Transition>> find_and_group_self_loops(const std::vector<Transition>& transitions) {
    std::unordered_map<std::string, std::list<Transition>> loops_by_state;

    for (const auto& transition : transitions) {
        if (transition.current_state == transition.next_state) {
            loops_by_state[transition.current_state].push_back(transition);
        }
    }

    std::vector<std::list<Transition>> result;

    for (auto& [state, loops] : loops_by_state) {
        if (!loops.empty()) {
            std::list<Transition> chain;
            for (auto& loop : loops) {
                chain.push_back(loop);
            }
            result.push_back(chain);
        }
    }

    return result;
}

std::vector<std::list<Transition>> merge_loops_and_singles(const std::vector<Transition>& singles, const std::vector<std::list<Transition>>& self_loops_chains) {
    std::vector<std::list<Transition>> result;

    for (const auto& single : singles) {
        result.push_back({single});
    }

    result.insert(result.end(), self_loops_chains.begin(), self_loops_chains.end());

    return result;
}

// для добавления элементов вида q_i -> q_i в цепочки одинарных звеньев
void insert_self_loop_transitions(std::vector<std::list<Transition>>& linked_lists, const std::vector<Transition>& self_loops) {
    if (self_loops.empty()) {
        // Если self_loops пуст, ничего не делаем
        return;
    }

    for (const auto& loop : self_loops) {
        bool inserted = false;

        for (auto& list : linked_lists) {
            if (inserted) break;

            auto it = list.begin();
            while (it != list.end()) {
                if (it->next_state == loop.current_state) {
                    list.insert(std::next(it), loop);
                    inserted = true;
                    break;
                } else if (it->current_state == loop.next_state) {
                    list.insert(it, loop);
                    inserted = true;
                    break;
                }
                ++it;
            }
        }

        if (!inserted) {
            linked_lists.push_back({loop});
        }
    }
}

std::vector<Transition> multies_to_singles(const std::unordered_map<std::string, std::vector<Transition>>& multies) {

    std::vector<Transition> result;
    for (const auto& pair : multies) {
        const std::vector<Transition>& transitions = pair.second;
        for (const auto& transition : transitions) {
            result.push_back(transition);
        }
    }

    return result;
}

// данная функция присоединяет одноразветвленные к выходам многоразветвленных
std::vector<std::list<Transition>> combine_multies_with_singles(std::unordered_map<std::string, std::vector<Transition>> multi_branched, std::vector<std::list<Transition>>& single_chains, std::string initial_state) {
    std::vector<std::list<Transition>> result;
    std::unordered_set<Transition> added_set;

    // изначально нужно перенести в итоговый вектор все цепочки, которые начинаются с initial_state (иначе потом они могут прикрепиться к звеньям, которые не начинаются с initial_state)
    // сначала одноразветвленные
    auto it_single = single_chains.begin();
    while (it_single != single_chains.end()) {
        if (!it_single->empty() && it_single->front().current_state == initial_state) {
            result.push_back(*it_single);
            it_single = single_chains.erase(it_single);
        } else {
            ++it_single;
        }
    }
    // теперь многоразветвленные (если одноразветвленных с началом initial_state нет)
    auto it = multi_branched.begin();
    while (it != multi_branched.end()) {
        if (it->first == initial_state) {
            std::list<Transition> transitions(it->second.begin(), it->second.end());
            result.push_back(std::move(transitions));
            it = multi_branched.erase(it);
        } else {
            ++it;
        }
    }

    for (auto& [state, transitions] : multi_branched) {
        for (auto& transition : transitions) {
            // нужно понять, возможно ли связать концы многоразветвленного с какими-либо началами одноразветвленных цепей
            auto connection = transition.next_state;
            auto is_connectible = std::find_if(single_chains.begin(), single_chains.end(),
                [&connection](const std::list<Transition>& chain) {
                    return !chain.empty() && chain.front().current_state == connection;
                });

            // если нашли одноразветвленную, которую можно связать с одним из концов многоразветвленной, то этого еще не достаточно
            if (is_connectible != single_chains.end()) {

                // узнаем на что он оканчивается
                auto ref_end = is_connectible->back().next_state;

                // нужно проверить, чтобы концом не ссылался на начало многоразветвленного
                if (ref_end != transitions.front().current_state) {
                    auto tmp_chain = *is_connectible;
                    tmp_chain.push_front(transition);
                    result.push_back(tmp_chain);
                    added_set.insert(transition);
                    single_chains.erase(is_connectible);
                }
                // если он все же ссылается, то его нельзя присоединять к одному из концов многоразветвленной, а нужно присоеденить к началу
                else {
                    // в данном случае мы его присоеденим в нчало для всех переходов данного состояния
                    for (auto& elem : transitions) {
                        auto tmp_chain = *is_connectible;
                        tmp_chain.push_back(elem);
                        result.push_back(tmp_chain);
                        added_set.insert(elem);
                    }
                    single_chains.erase(is_connectible);
                    continue;
                }
            }
        }
    }

    // также в конце всего нужно не забыть добавить недобавленные одиночные цепи и неиспользованные части многоразветвленных цепей
    for (auto& remaining : single_chains) {
        result.push_back(remaining);
    }
    for (auto& [state, transitions] : multi_branched) {
        for (auto& transition : transitions) {
            if (added_set.find(transition) == added_set.end()) {
                result.push_back({transition});
            }
        }
    }

    return result;
}

// данная функция присоединяет многоразветвленные к выходам одноразветвленных
std::vector<std::list<Transition>> combine_singles_with_multies(std::vector<std::list<Transition>>& single_chains, std::unordered_map<std::string, std::vector<Transition>> multi_branched) {
    std::vector<std::list<Transition>> result;

    for (auto& [state, transitions] : multi_branched) {
        // нужно понять, возможно ли связать начала многоразветвленного с каким-либо концом одноразветвленных цепей
        auto is_connectible = find_list_ending_with(single_chains, state);

        if (is_connectible) {
            for (auto& transition : transitions) {
                auto tmp_chain = *is_connectible;
                tmp_chain.push_back(transition);
                result.push_back(tmp_chain);
            }

            // т.к. уже присоеденили одиночную цепочку, то в дальнейшем ей пользоваться нельзя и ее нужно убрать
            auto it = std::find_if(single_chains.begin(), single_chains.end(), [&](const std::list<Transition>& chain) {
                return &chain == is_connectible;
            });
            single_chains.erase(it);

        } else {
            for (auto& transition : transitions) {
                std::list<Transition> inconnectible = {transition};
                result.push_back(inconnectible);
            }
        }
    }

    // также в конце всего нужно не забыть добавить недобавленные одиночные цепи
    for (auto& remaining : single_chains) {
        result.push_back(remaining);
    }
    return result;
}

bool has_non_empty_vectors(const std::unordered_map<std::string, std::vector<Transition>>& map) {
    for (const auto& pair : map) {
        if (!pair.second.empty()) {
            return true;
        }
    }
    return false;
}

bool all_singles_used(const std::vector<std::list<Transition>>& chains_of_singles, const std::unordered_set<std::list<Transition>, Transition::ListHash>& used_single_chains) {
    for (const auto& chain : chains_of_singles) {
        if (used_single_chains.find(chain) == used_single_chains.end()) {
            return false;
        }
    }
    return true;
}

bool all_multies_used(const std::unordered_map<std::string, std::vector<Transition>>& multi_branched, const std::unordered_set<std::string>& used_multi_branches) {
    for (const auto& pair : multi_branched) {
        const std::string& key = pair.first;

        if (used_multi_branches.find(key) == used_multi_branches.end()) {
            return false;
        }
    }
    return true;
}

std::vector<std::list<Transition>> connect_everything(std::unordered_map<std::string, std::vector<Transition>>& multi_branched, std::vector<std::list<Transition>>& chains_of_singles, const std::string& initial_state) {
    std::vector<std::list<Transition>> result;
    std::unordered_set<std::list<Transition>, Transition::ListHash> used_single_chains;
    std::unordered_set<std::string> used_multi_branches;

    // Находим корневые цепочки из chains_of_singles, которые начинаются с initial_state
    auto single_it = chains_of_singles.begin();
    while (single_it != chains_of_singles.end()) {
        if (!single_it->empty() && single_it->front().current_state == initial_state && used_single_chains.find(*single_it) == used_single_chains.end()) {
            result.push_back(*single_it);
            used_single_chains.insert({*single_it});
        } else {
            ++single_it;
        }
    }

    // Находим корневые цепочки из multi_branched, которые начинаются с initial_state
    auto multi_it = multi_branched.find(initial_state);
    if (multi_it != multi_branched.end()) {
        auto links = multi_it->second;
        for (auto& link : links) {
            result.push_back({link});
        }
        used_multi_branches.insert(initial_state);
    }

    if (result.empty()) {
        throw std::runtime_error("No root chains starting from the initial state.");
    }

    // Добавление цепочек из multi_branched и chains_of_singles
    auto restart_outer_loop = false;
    auto all_of_multies_used = all_multies_used(multi_branched, used_multi_branches);
    auto all_of_singles_used = all_singles_used(chains_of_singles, used_single_chains);
    while (!all_of_singles_used || !all_of_multies_used) {

        auto added = false;
        restart_outer_loop = false;

        std::sort(result.begin(), result.end(), [](const std::list<Transition>& a, const std::list<Transition>& b) {
            return a.size() < b.size();
        });

        for (auto it = result.begin(); it != result.end(); ++it) {
            if (it == result.end()) {
                break;
            }

            auto end_state = it->back().next_state;

            // Обработка цепочек из chains_of_singles
            for (auto sg_it = chains_of_singles.begin(); sg_it != chains_of_singles.end(); ++sg_it) {
                if (!sg_it->empty() && sg_it->front().current_state == end_state && used_single_chains.find(*sg_it) == used_single_chains.end()) {
                    auto new_chain = *it;
                    new_chain.insert(new_chain.end(), sg_it->begin(), sg_it->end());
                    result.push_back(new_chain);
                    used_single_chains.insert(*sg_it);
                    added = true;
                    restart_outer_loop = true;
                    break;
                }
            }
            all_of_singles_used = all_singles_used(chains_of_singles, used_single_chains);

            if (restart_outer_loop) {
                restart_outer_loop = false;
                break;
            }

            // Обработка цепочек из multi_branched
            if (used_multi_branches.find(end_state) == used_multi_branches.end()) {
                auto mb_it = multi_branched.find(end_state);
                if (mb_it != multi_branched.end()) {
                    used_multi_branches.insert(end_state);
                    auto& transitions = mb_it->second;
                    auto new_chn = *it;
                    for (auto& transition : transitions) {
                        auto new_chain = new_chn;
                        new_chain.push_back(transition);
                        result.push_back(new_chain);
                    }
                    added = true;
                    restart_outer_loop = true;
                    all_of_multies_used = all_multies_used(multi_branched, used_multi_branches);
                    break;
                }
            }
        }

        if (!added) {
            throw std::runtime_error("No chains could be connected in this iteration.");
        }
    }

    // Проверка, что все цепочки начинаются с начального состояния
    for (const auto& chain : result) {
        if (!chain.empty() && chain.front().current_state != initial_state) {
            throw std::runtime_error("Error! The result contains a chain that doesn't start from the initial state.");
        }
    }
    return result;
}

bool is_sublist(const std::list<Transition>& sub, const std::list<Transition>& full) {
    if (sub.size() > full.size()) {
        return false;
    }

    auto it_sub_start = sub.begin();
    auto it_full_start = full.begin();

    while (it_full_start != full.end()) {
        auto it_sub = it_sub_start;
        auto it_full = it_full_start;

        while (it_sub != sub.end() && it_full != full.end() && *it_sub == *it_full) {
            ++it_sub;
            ++it_full;
        }

        if (it_sub == sub.end()) {
            return true;
        }

        ++it_full_start;
    }

    return false;
}

void remove_sublists(std::vector<std::list<Transition>>& sequences) {
    std::vector<std::list<Transition>> result;

    std::sort(sequences.begin(), sequences.end(), [](const std::list<Transition>& a, const std::list<Transition>& b) {
        return a.size() > b.size();
    });

    for (const auto& seq : sequences) {
        bool is_sub = false;
        for (const auto& other : result) {
            if (is_sublist(seq, other)) {
                is_sub = true;
                break;
            }
        }
        if (!is_sub) {
            result.push_back(seq);
        }
    }
    sequences = std::move(result);
}

bool all_transitions_present(const std::vector<Transition>& transitions, const std::vector<std::list<Transition>>& result_sequences) {

    std::unordered_set<Transition, Transition::Hash> transition_set;

    for (const auto& chain : result_sequences) {
        for (const auto& transition : chain) {
            transition_set.insert(transition);
        }
    }

    for (const auto& transition : transitions) {
        if (transition_set.find(transition) == transition_set.end()) {
            return false;
        }
    }

    return true;
}

std::vector<std::list<Transition>> filter(const std::vector<std::list<Transition>>& unfiltered_sequences, const std::vector<Transition>& all_transitions) {
    std::unordered_set<Transition, Transition::Hash> unique_transitions;
    std::vector<std::list<Transition>> filtered_sequences;

    // сначала нужно отфильтровать все последовательности, чтобы не было тех, которые не добавляют новых переходов
    for (const auto& sequence : unfiltered_sequences) {
        auto has_new_transition = false;

        for (const auto& transition : sequence) {
            if (unique_transitions.find(transition) == unique_transitions.end()) {
                unique_transitions.insert(transition);
                has_new_transition = true;
            }
        }

        if (has_new_transition) {
            filtered_sequences.push_back(sequence);
        }
    }
    return filtered_sequences;
}

void remove_non_unique_substrings(std::vector<std::vector<std::string>>& sequences) {
    std::unordered_set<std::string> unique_sequences;

    for (auto& sequence : sequences) {
        std::string combined_sequence = "";
        for (const auto& element : sequence) {
            combined_sequence += element + ",";
        }
        if (unique_sequences.find(combined_sequence) == unique_sequences.end()) {
            unique_sequences.insert(combined_sequence);
        } else {
            sequence.clear();
        }
    }

    sequences.erase(std::remove_if(sequences.begin(), sequences.end(),
                        [](const std::vector<std::string>& sequence) { return sequence.empty(); }),
        sequences.end());
}

int find_max_path_len(const pt::ptree& transitions, const std::string& current_state, std::unordered_map<std::string, int>& memo, int input_length, std::unordered_set<std::string>& visited) {
    // Проверка на зацикливание
    if (visited.find(current_state) != visited.end()) {
        return input_length;
    }

    visited.insert(current_state);

    // Проверка, вычисляли ли уже максимальную длину пути для этого состояния
    if (memo.find(current_state) != memo.end()) {
        visited.erase(current_state);
        return memo[current_state];
    }

    int max_len = 0;
    auto transitions_opt = transitions.get_child_optional(current_state);
    if (!transitions_opt) {
        // Если нет переходов из текущего состояния, длина пути равна 0
        visited.erase(current_state);
        return max_len;
    }

    for (const auto& transition : transitions_opt.get()) {
        // Проверка наличия следующего состояния
        if (!transition.second.get_optional<std::string>("state")) {
            continue; // Если ключа "state" нет, пропускаем
        }

        std::string next_state = transition.second.get<std::string>("state");
        // Рекурсивный вызов для следующего состояния
        // для предотвращения бесконечной рекурсии
        if (max_len == input_length) {
            return input_length;
        }

        int len = 1 + find_max_path_len(transitions, next_state, memo, input_length, visited);
        max_len = std::max(max_len, len);
    }

    // Сохраняем результат в memo, чтобы избежать повторных вычислений
    memo[current_state] = max_len;
    visited.erase(current_state);
    return max_len;
}