#include <algorithm>
#include <boost/program_options.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <fstream>
#include <iostream>
#include <iterator>
#include <random>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace pt = boost::property_tree;
namespace po = boost::program_options;

struct generator_parameters {
    unsigned int seed;
    unsigned int n_states_min;
    unsigned int n_states_max;
    unsigned int n_alph_in_min;
    unsigned int n_alph_in_max;
    unsigned int n_alph_out_min;
    unsigned int n_alph_out_max;
    unsigned int n_trans_out_min;
    unsigned int n_trans_out_max;
    std::string output_file;
};

std::vector<std::string> create_sequence_q_i(int n_states) {
    std::vector<std::string> sequence;
    for (int i = 1; i <= n_states; ++i) {
        sequence.push_back("q" + std::to_string(i));
    }
    return sequence;
}

int generate_machine(const generator_parameters &params) {

    // проверка введеных пользователем значений диапазонов (явные ошибки)
    if (params.n_states_min > params.n_states_max) {
        std::cerr << "incorrect range for states" << std::endl;
        return 1;
    }
    if (params.n_alph_in_min > params.n_alph_in_max) {
        std::cerr << "incorrect range for alph_in" << std::endl;
        return 1;
    }
    if (params.n_alph_out_min > params.n_alph_out_max) {
        std::cerr << "incorrect range for alph_out" << std::endl;
        return 1;
    }
    if (params.n_trans_out_min > params.n_trans_out_max) {
        std::cerr << "incorrect range for trans_out" << std::endl;
        return 1;
    }
    if (params.n_alph_in_min == 0 || params.n_alph_out_min == 0 || params.n_states_min == 0) {
        std::cerr << "alph_in, alph_out, n_states can't be = 0" << std::endl;
        return 1;
    }
    if (params.n_states_min != 0 && params.n_trans_out_max == 0) {
        std::cerr << "incorrect combination between n_states_min > 0 and n_trans_out_max = 0" << std::endl;
        return 1;
    }

    // если вдруг условие n_alph_in_max > n_trans_out_min нарушено, то детерминированного автомата вообще не может существовать
    if (params.n_alph_in_max < params.n_trans_out_min) {
        std::cerr << "incorrect combination between alph_in_max and trans_out_min - condition n_alph_in_max > n_trans_out_min is violated!!!" << std::endl;
        return 1;
    }

    // число состояний, мощности алфавитов входных и выходных символов - общие для всего автомата
    std::mt19937 gen(params.seed);
    std::uniform_int_distribution<unsigned int> dist_n_states(params.n_states_min, params.n_states_max);
    std::uniform_int_distribution<unsigned int> dist_n_alph_in(params.n_alph_in_min, params.n_alph_in_max);
    std::uniform_int_distribution<unsigned int> dist_n_alph_out(params.n_alph_out_min, params.n_alph_out_max);
    auto n_states = dist_n_states(gen);
    auto n_alph_in = dist_n_alph_in(gen);
    auto n_alph_out = dist_n_alph_out(gen);

    // регенерация мощности входного алфавита, если вдруг сгенерированое n_alph_in < n_trans_out_min
    while (n_alph_in < params.n_trans_out_min) {

        n_alph_in = dist_n_alph_in(gen);
    }

    auto upper_bound_trans_out = std::min(params.n_trans_out_max, n_alph_in); // условие на недетерминированность исходящих переходов
    std::uniform_int_distribution<unsigned int> dist_n_trans_out(params.n_trans_out_min, upper_bound_trans_out);

    std::uniform_int_distribution<unsigned int> dist_symb_out(1, n_alph_out);
    std::uniform_int_distribution<unsigned int> dist_symb_in(1, n_alph_in);

    pt::ptree tree;
    pt::ptree transitions;

    // изначально нужно сгенерировать связный "каркас"
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // стратегия построения связного каркаса
    /*
    Есть два типа состояний:
    1. Доноры - у них n_trans_out >= 1
    2. Акцепторы - у них n_trans_out = 0
    Т.о. можно получить 1е необходимое условие связности: сумма всех n_trans_out >= n_states - 1

    Также можно выявить 2е необходимое условие связности: в каждое состояние входит как минимум 1 переход.
    Эти два условия не являются достаточными, т.к. можно построить простейший контрпример: q1->q2 q2->q1 q3->q4 q4->q3 - не связный автомат, хотя все два условия выполнены (далее эту проблему я буду называть "попарными взаимными ссылками").

    Для решения данной проблемы можно использовать итерацию по состояниям от верхнего уровня к нижнему уровню + разбитие на доменно-акцепторные группы и их связывание между собой (тогда у нас не будет взаимных попарных ссыланий, т.к. направление обхода идет только в одну сторону)
    */

    auto all_qi = create_sequence_q_i(n_states);
    std::shuffle(all_qi.begin(), all_qi.end(), gen);

    // для хранения информации об исходящих переходах
    std::unordered_map<std::string, unsigned int> output_transitions;
    // для хранения доноров
    std::vector<std::string> donors;
    // для хранения акцепторов
    std::vector<std::string> acceptors;
    // счетчик суммы всех n_trans_out
    auto sum_all_trans_out = 0;

    // для работы с группами соединений (ключ - название донора, значения - массив акцепторов)
    std::unordered_map<std::string, std::vector<std::string>> groups;

    auto first_necessary_condition = false;
    while (!first_necessary_condition) {

        output_transitions.clear();
        donors.clear();
        acceptors.clear();
        sum_all_trans_out = 0;

        for (auto &&qi : all_qi) {
            // нам нужно понять донор или акцептор, для этого нужно узнать число n_trans_out
            auto n_trans_out = dist_n_trans_out(gen);
            if (n_trans_out > 0) {

                donors.push_back(qi);
                output_transitions.insert(std::make_pair(qi, n_trans_out));
                sum_all_trans_out += n_trans_out;

            } else {

                acceptors.push_back(qi);
                output_transitions.insert(std::make_pair(qi, n_trans_out));
            }
        }
        if (sum_all_trans_out >= n_states) {
            first_necessary_condition = true;
        }
    }

    for (size_t i = 0; i < all_qi.size(); i++) {
        ////// отслеживание входных символов перехода
        std::set<std::string> used_in_symbols;
        auto qi_opt = transitions.get_child_optional(all_qi[i]);
        pt::ptree state_transitions;
        if (qi_opt) {
            auto &qi_node = *qi_opt;
            for (auto &&zi : qi_node) {

                used_in_symbols.insert(zi.first);
                state_transitions = qi_node;
            }
        }

        auto o_trs = output_transitions[all_qi[i]];

        // если это донор, то добавляем его
        if (o_trs > 0) {
            groups.insert(std::make_pair(all_qi[i], std::vector<std::string>()));
        }

        // отдавать переходы можно до того момента, пока число валентных переходов не станет = 1 (не 0, т.к. это связано с особенностью на следующем этапе алгоритма)
        while (o_trs > 1) {

            output_transitions[all_qi[i]] -= 1;
            o_trs = output_transitions[all_qi[i]];

            std::string symb_in;
            while (used_in_symbols.find(symb_in) != used_in_symbols.end() || symb_in.empty()) {
                symb_in = "z" + std::to_string(dist_symb_in(gen));
            };
            used_in_symbols.insert(symb_in);

            std::string symb_out = "w" + std::to_string(dist_symb_out(gen));
            std::string next_state;

            /*
            пока не закончились акцепторы, доноры будут отдавать им переходы
            т.о. будут созданы группы из одного донора и акцепторов
            важно понимать, что эти группы могут не пересекаться, поэтому задача пересечения - следующий этап
            */
            if (!acceptors.empty()) {

                next_state = acceptors.back();
                acceptors.pop_back();
                groups[all_qi[i]].push_back(next_state);
            }
            // если акцепторы закончились, то переходим к тому следующему этапу
            else {
                break;
            }

            pt::ptree transition;
            transition.put("state", next_state);
            transition.put("output", symb_out);

            state_transitions.add_child(symb_in, transition);
            transitions.put_child(all_qi[i], state_transitions);
        }
    }

    /*
    следующий этап предполагает связывание групп из прошлого этапа
    донор группы n может перекинуть переход на любой элемент группы n+1
    перекидывание перехода возможно, т.к. после прошлого этапа у каждого донора остался хотя бы 1 валентный переход
    */

    auto sz = groups.size();
    auto ind = 0;
    for (auto it = groups.begin(); it != groups.end(); ++it) {

        pt::ptree transition;
        pt::ptree state_transitions;

        std::set<std::string> used_in_symbols;

        auto qi_opt = transitions.get_child_optional(it->first);

        if (qi_opt) {
            auto &qi_node = *qi_opt;
            for (auto &&zi : qi_node) {
                used_in_symbols.insert(zi.first);
                state_transitions = qi_node;
            }
        }

        std::string symb_in;
        while (used_in_symbols.find(symb_in) != used_in_symbols.end() || symb_in.empty()) {
            symb_in = "z" + std::to_string(dist_symb_in(gen));
        };
        used_in_symbols.insert(symb_in);

        std::string symb_out = "w" + std::to_string(dist_symb_out(gen));
        std::string next_state;

        // т.к. перекидываем переход на последующую группу, нужно учитывать, чтобы не зайти за пределы
        if (ind < sz - 1) {

            auto tmp_it = it;
            auto next_it = ++tmp_it;

            auto &next_donor = next_it->first;
            next_state = next_donor;

        }
        // если перекидываем из последней группы, то перекидываем на любую из предыдущих
        else {

            std::uniform_int_distribution<> dist_group(0, groups.size() - 1);
            auto rand_group = dist_group(gen);

            auto other_it = groups.begin();
            std::advance(other_it, rand_group);
            std::uniform_int_distribution<> dist_choice(0, other_it->second.size());
            auto choice = dist_choice(gen);

            if (choice == 0) {
                next_state = other_it->first;
            } else {
                next_state = other_it->second[choice - 1];
            }
        }
        output_transitions[it->first] -= 1;
        ind++;

        transition.put("state", next_state);
        transition.put("output", symb_out);

        state_transitions.add_child(symb_in, transition);
        transitions.put_child(it->first, state_transitions);
    }

    // теперь, когда связный каркас сгенерирован, можно поперебрасывать оставшиеся переходы
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////

    for (auto &&qi : all_qi) {

        pt::ptree transition;
        pt::ptree state_transitions;

        std::set<std::string> used_in_symbols;
        auto qi_opt = transitions.get_child_optional(qi);
        if (qi_opt) {
            auto &qi_node = *qi_opt;
            for (auto &&zi : qi_node) {
                used_in_symbols.insert(zi.first);
                state_transitions = qi_node;
            }
        }

        // перебрасывать оставшиеся переходы можно только у тех состояний, у которых они остались
        while (output_transitions[qi] > 0) {
            std::string symb_in;
            while (used_in_symbols.find(symb_in) != used_in_symbols.end() || symb_in.empty()) {
                symb_in = "z" + std::to_string(dist_symb_in(gen));
            }
            used_in_symbols.insert(symb_in);

            std::string symb_out = "w" + std::to_string(dist_symb_out(gen));
            std::uniform_int_distribution<> dist_in_all_qi(0, all_qi.size() - 1);
            std::string next_state;

            next_state = all_qi[dist_in_all_qi(gen)];

            output_transitions[qi] -= 1;

            pt::ptree state_transition;
            state_transition.put("state", next_state);
            state_transition.put("output", symb_out);

            state_transitions.add_child(symb_in, state_transition);
            transitions.put_child(qi, state_transitions);
        }
    }

    tree.put("initial_state", groups.begin()->first);
    tree.add_child("transitions", transitions);

    std::ofstream output(params.output_file);
    if (!output.is_open()) {
        std::cerr << "Error opening output file!!!" << std::endl;
        return 2;
    }
    pt::write_json(output, tree);
    return 0;
}

int main(int argc, char *argv[]) {
    try {
        po::options_description desc("Allowed options");
        desc.add_options()("help", "produce help message")("seed", po::value<unsigned int>()->required(), "set random seed")("n_states_min", po::value<unsigned int>()->required(), "set minimum number of states")("n_states_max", po::value<unsigned int>()->required(), "set maximum number of states")("n_alph_in_min", po::value<unsigned int>()->required(), "set minimum input alphabet size")("n_alph_in_max", po::value<unsigned int>()->required(), "set maximum input alphabet size")("n_alph_out_min", po::value<unsigned int>()->required(), "set minimum output alphabet size")("n_alph_out_max", po::value<unsigned int>()->required(), "set maximum output alphabet size")("n_trans_out_min", po::value<unsigned int>()->required(), "set minimum number of outgoing transitions")("n_trans_out_max", po::value<unsigned int>()->required(), "set maximum number of outgoing transitions")("out", po::value<std::string>()->required(), "set output file name");

        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);

        if (vm.count("help")) {
            std::cout << desc << "\n";
            return 0;
        }

        po::notify(vm);

        generator_parameters params;
        params.seed = vm["seed"].as<unsigned int>();
        params.n_states_min = vm["n_states_min"].as<unsigned int>();
        params.n_states_max = vm["n_states_max"].as<unsigned int>();
        params.n_alph_in_min = vm["n_alph_in_min"].as<unsigned int>();
        params.n_alph_in_max = vm["n_alph_in_max"].as<unsigned int>();
        params.n_alph_out_min = vm["n_alph_out_min"].as<unsigned int>();
        params.n_alph_out_max = vm["n_alph_out_max"].as<unsigned int>();
        params.n_trans_out_min = vm["n_trans_out_min"].as<unsigned int>();
        params.n_trans_out_max = vm["n_trans_out_max"].as<unsigned int>();
        params.output_file = vm["out"].as<std::string>();

        return generate_machine(params);

    } catch (const po::error &e) {
        std::cerr << "Command line error: " << e.what() << "\n";
        return 2;
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 2;
    }
}
