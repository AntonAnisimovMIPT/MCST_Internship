#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/program_options.hpp>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>
#include <random>
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

auto create_sequence_q_i(int n_states) {
    std::vector<std::string> sequence;
    for (int i = 1; i <= n_states; ++i) {
        sequence.push_back("q" + std::to_string(i));
    }
    return sequence;
}

auto create_input_symbols(int lower_bound, int upper_bound) {
    std::vector<std::string> input_symbols;
    for (int i = lower_bound; i <= upper_bound; ++i) {
        input_symbols.push_back("z" + std::to_string(i));
    }
    return input_symbols;
}

int generate_machine(const generator_parameters& params) {

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
    std::uniform_int_distribution<unsigned int> dist_n_alph_out(params.n_alph_out_min, params.n_alph_out_max);
    auto n_states = dist_n_states(gen);
    auto n_alph_out = dist_n_alph_out(gen);

    auto lower_bound_alph_in_min = std::max(params.n_alph_in_min, params.n_trans_out_min); // условие на недетерминированность исходящих переходов
    std::uniform_int_distribution<unsigned int> dist_n_alph_in(lower_bound_alph_in_min, params.n_alph_in_max);
    auto n_alph_in = dist_n_alph_in(gen);

    auto upper_bound_trans_out = std::min(params.n_trans_out_max, n_alph_in); // условие на недетерминированность исходящих переходов
    // std::uniform_int_distribution<unsigned int> dist_n_trans_out(params.n_trans_out_min, upper_bound_trans_out);

    std::uniform_int_distribution<unsigned int> dist_symb_out(1, n_alph_out);
    // std::uniform_int_distribution<unsigned int> dist_symb_in(1, n_alph_in);

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
    // счетчик состояний, которые могут быть уже связанными
    auto sum_all_trans_out = 0;

    // для работы с группами соединений (ключ - название донора, значения - массив акцепторов)
    std::unordered_map<std::string, std::vector<std::string>> groups;

    // для хранения информации о неиспользованных входных символах
    std::unordered_map<std::string, std::vector<std::string>> unused_input_symbols;
    std::string ac_tmp;

    // приступаем к генерации "на ходу"
    // изначально нужно понять, сколько исходящих переходов у каждого состояния
    for (size_t i = 0; i < all_qi.size(); i++) {

        auto lower_bound_trans_out = 0;
        // если вдруг есть еще несвязанные состояния, то нижняя граница генерации 1
        if (sum_all_trans_out < all_qi.size() - 1) {

            lower_bound_trans_out = 1;
        }

        // генерируем число исходящих переходов для текущего состояния
        std::uniform_int_distribution<unsigned int> dist_n_trans_out(lower_bound_trans_out, upper_bound_trans_out);
        auto n_trans_out = dist_n_trans_out(gen);

        sum_all_trans_out += n_trans_out;

        // нам нужно понять донор или акцептор, для этого нужно узнать число n_trans_out
        if (n_trans_out > 0) {
            donors.push_back(all_qi[i]);
        } else {
            acceptors.push_back(all_qi[i]);
            ac_tmp = all_qi[i];
        }
        output_transitions.insert(std::make_pair(all_qi[i], n_trans_out));
    }

    // теперь осталось связать каркас
    for (size_t i = 0; i < all_qi.size(); i++) {

        pt::ptree state_transitions;

        ////// отслеживание входных символов перехода
        auto in_symbs = create_input_symbols(1, n_alph_in);
        std::shuffle(in_symbs.begin(), in_symbs.end(), gen);
        in_symbs.resize(output_transitions.find(all_qi[i])->second);
        unused_input_symbols.insert({all_qi[i], in_symbs});

        auto o_trs = output_transitions.find(all_qi[i])->second;

        // если это донор, то добавляем его
        if (o_trs > 0) {
            groups.insert(std::make_pair(all_qi[i], std::vector<std::string>()));
        }

        // отдавать переходы можно до того момента, пока число валентных переходов не станет = 1 (не 0, т.к. это связано с особенностью на следующем этапе алгоритма)
        while (o_trs > 1) {

            output_transitions.find(all_qi[i])->second -= 1;
            o_trs = output_transitions.find(all_qi[i])->second;

            auto it_s = unused_input_symbols.find(all_qi[i]);
            std::string symb_in;
            if (it_s != unused_input_symbols.end()) {
                symb_in = it_s->second.back();
                it_s->second.pop_back();
            } else {
                throw std::runtime_error("it_s == unused_input_symbols.end()");
            }

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

        auto it_s = unused_input_symbols.find(it->first);
        std::string symb_in;
        if (it_s != unused_input_symbols.end()) {
            symb_in = it_s->second.back();
            it_s->second.pop_back();
        } else {
            throw std::runtime_error("it_s == unused_input_symbols.end()");
        }

        std::string symb_out = "w" + std::to_string(dist_symb_out(gen));
        std::string next_state;

        // т.к. перекидываем переход на последующую группу, нужно учитывать, чтобы не зайти за пределы
        if (ind < sz - 1) {

            auto tmp_it = it;
            auto next_it = ++tmp_it;

            auto& next_donor = next_it->first;
            next_state = next_donor;
            output_transitions.find(it->first)->second -= 1;
            transition.put("state", next_state);
            transition.put("output", symb_out);
            state_transitions.add_child(symb_in, transition);

        }
        // если перекидываем из последней группы, то перекидываем на любую из предыдущих
        else {

            // для последнего состояния можем перекинуть, а можем и не перекинуть
            std::uniform_int_distribution<> need_transfer(0, 1);
            auto is_transfer = need_transfer(gen);

            // если перекидываем
            if (is_transfer == true) {
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
                output_transitions.find(it->first)->second -= 1;
                transition.put("state", next_state);
                transition.put("output", symb_out);
                state_transitions.add_child(symb_in, transition);
            }
        }
        ind++;

        transitions.put_child(it->first, state_transitions);
    }

    // теперь, когда связный каркас сгенерирован, можно поперебрасывать оставшиеся переходы
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // std::cout << "go to perebros\n";
    for (const auto& qi : all_qi) {

        // перебрасывать оставшиеся переходы можно только у тех состояний, у которых они остались
        if (unused_input_symbols.find(qi) != unused_input_symbols.end()) {
            // std::cout << "go to if 1\n";
            //  Загружаем уже существующие переходы для состояния qi
            pt::ptree state_transitions;
            if (transitions.find(qi) != transitions.not_found()) {
                state_transitions = transitions.get_child(qi);
            }
            // std::cout << "go to if 2\n";

            while (!unused_input_symbols[qi].empty()) {

                auto it_s = unused_input_symbols.find(qi);
                std::string symb_in;
                if (it_s != unused_input_symbols.end()) {
                    // std::cout << "go to if 3\n";

                    symb_in = it_s->second.back();
                    it_s->second.pop_back();

                } else {
                    throw std::runtime_error("it_s == unused_input_symbols.end()");
                }

                std::string symb_out = "w" + std::to_string(dist_symb_out(gen));
                std::uniform_int_distribution<> dist_in_all_qi(0, all_qi.size() - 1);
                // std::cout << "go to a\n";
                auto next_state = all_qi[dist_in_all_qi(gen)];
                output_transitions[qi] -= 1;

                pt::ptree state_transition;
                state_transition.put("state", next_state);
                state_transition.put("output", symb_out);
                // std::cout << "go to b\n";
                state_transitions.add_child(symb_in, state_transition);
                // std::cout << "go to c\n";
            }

            // Обновляем переходы для состояния qi в дереве transitions
            transitions.put_child(qi, state_transitions);
            // std::cout << "go to put_child\n";
        }
    }
    /*
    std::cout << "go to put initial\n";
    std::cout << "groups size " << groups.size() << std::endl;
    for (auto& [st, trs] : groups) {
        std::cout << st << std::endl;
    }
    */
    if (groups.size() == 0) {
        tree.put("initial_state", ac_tmp);
        tree.add_child("transitions", transitions);
        std::ofstream output(params.output_file);
        if (!output.is_open()) {
            std::cerr << "Error opening output file!!!" << std::endl;
            return 2;
        }
        pt::write_json(output, tree);
        return 0;
    }

    tree.put("initial_state", groups.begin()->first);
    // std::cout << "go to add transitions\n";
    tree.add_child("transitions", transitions);

    std::ofstream output(params.output_file);
    if (!output.is_open()) {
        std::cerr << "Error opening output file!!!" << std::endl;
        return 2;
    }
    pt::write_json(output, tree);
    return 0;
}

int main(int argc, char* argv[]) {
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

    } catch (const po::error& e) {
        std::cerr << "Command line error: " << e.what() << "\n";
        return 2;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 2;
    }
}