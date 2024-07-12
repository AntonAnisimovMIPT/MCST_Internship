#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/program_options.hpp>
#include <random>
#include <iostream>
#include <fstream>
#include <set>
#include <vector>
#include <unordered_map>
#include <algorithm>

namespace pt = boost::property_tree;
namespace po = boost::program_options;

struct generator_parameters 
{

    int seed;

    int n_states_min;
    int n_states_max;

    int n_alph_in_min;
    int n_alph_in_max;

    int n_alph_out_min;
    int n_alph_out_max;

    int n_trans_out_min;
    int n_trans_out_max;

    std::string output_file;

};

std::vector<std::string> create_sequence_q_i(int n_states) 
{
    std::vector<std::string> sequence;
    for (int i = 1; i <= n_states; ++i) 
    {
        sequence.push_back("q" + std::to_string(i));
    }
    return sequence;
}

int generate_machine(const generator_parameters &params) 
{

    // проверка введеных пользователем значений диапазонов
    if (params.n_states_min > params.n_states_max)
    {
        std::cerr << "incorrect range for states" << std::endl;
        return 1;
    }
    if (params.n_alph_in_min > params.n_alph_in_max)
    {
        std::cerr << "incorrect range for alph_in" << std::endl;
        return 1;
    }
    if (params.n_alph_out_min > params.n_alph_out_max)
    {
        std::cerr << "incorrect range for alph_out" << std::endl;
        return 1;
    }
    if (params.n_trans_out_min > params.n_trans_out_max)
    {
        std::cerr << "incorrect range for trans_out" << std::endl;
        return 1;
    }
        if (params.n_states_min <= 0 || params.n_states_max <= 0)
    {
        std::cerr << "non - positive bound for states_min or states_max" << std::endl;
        return 1;
    }
    if (params.n_alph_in_min <= 0 || params.n_alph_in_max <= 0)
    {
        std::cerr << "non - positive bound for alph_in_min or alph_in_max" << std::endl;
        return 1;
    }
    if (params.n_alph_out_min <= 0 || params.n_alph_out_max <= 0)
    {
        std::cerr << "non - positive bound for alph_out_min or alph_out_max" << std::endl;
        return 1;
    }
    if (params.n_trans_out_min <= 0 || params.n_trans_out_max <= 0)
    {
        std::cerr << "non - positive bound for trans_out_min or trans_out_max" << std::endl;
        return 1;
    }

    // если вдруг условие n_alph_in_max > n_trans_out_min нарушено, то детерминированного автомата вообще не может существовать
    if (params.n_alph_in_max < params.n_trans_out_min)
    {
        std::cerr << "incorrect combination between alph_in_max and trans_out_min - condition n_alph_in_max > n_trans_out_min is violated!!!"  << std::endl;
        return 1;
    }


    
    // число состояний, мощности алфавитов входных и выходных символов - общие для всего автомата
    std::mt19937 gen(params.seed);
    std::uniform_int_distribution<> dist_n_states(params.n_states_min, params.n_states_max);
    std::uniform_int_distribution<> dist_n_alph_in(params.n_alph_in_min, params.n_alph_in_max);
    std::uniform_int_distribution<> dist_n_alph_out(params.n_alph_out_min, params.n_alph_out_max);
    auto n_states = dist_n_states(gen);
    auto n_alph_in = dist_n_alph_in(gen);
    auto n_alph_out = dist_n_alph_out(gen);

    // регенерация мощности входного алфавита, если вдруг сгенерированое n_alph_in < n_trans_out_min
    while (n_alph_in < params.n_trans_out_min)
    {
        n_alph_in = dist_n_alph_in(gen);
    }
    
    auto upper_bound_trans_out = std::min(params.n_trans_out_max, n_alph_in); // условие на недетерминированность исходящих переходов
    std::uniform_int_distribution<> dist_n_trans_out(params.n_trans_out_min, upper_bound_trans_out);

    std::uniform_int_distribution<> dist_symb_out(1, n_alph_out);
    std::uniform_int_distribution<> dist_symb_in(1, n_alph_in);

    pt::ptree tree;
    tree.put("initial_state", "q1");

    pt::ptree transitions;
    
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // изначально нужно сгенерировать связный "каркас" (т.е. тот, у которого в каждое состояние ведет один переход; круговая зависимость - единственный возможный сопсоб)
    // создадим круговую последовательность состояний и сделаем ее неупорядоченной
    auto all_qi = create_sequence_q_i(n_states);
    std::mt19937 g(params.seed);
    std::shuffle(all_qi.begin(), all_qi.end(), g);
    
    for (auto i = 0; i < n_states; ++i)
    {
        if (i == n_states - 1)
        {
            std::string current_state = all_qi[0];
            std::string donor_state = all_qi[i];

            // задание символов перехода
            std::string symb_in_donor_state = "z" + std::to_string(dist_symb_in(gen));
            std::string symb_out_donor_state = "w" + std::to_string(dist_symb_out(gen));

            // задание перехода
            pt::ptree transition;
            transition.put("state", current_state);
            transition.put("output", symb_out_donor_state);
            pt::ptree state_transitions;
            state_transitions.add_child(symb_in_donor_state, transition);
            transitions.add_child(donor_state, state_transitions);

        }
        else 
        {
        
            std::string current_state = all_qi[i+1];
            std::string donor_state = all_qi[i];

            // задание символов перехода
            std::string symb_in_donor_state = "z" + std::to_string(dist_symb_in(gen));
            std::string symb_out_donor_state = "w" + std::to_string(dist_symb_out(gen));

            // задание перехода
            pt::ptree transition;
            transition.put("state", current_state);
            transition.put("output", symb_out_donor_state);
            pt::ptree state_transitions;
            state_transitions.add_child(symb_in_donor_state, transition);
            transitions.add_child(donor_state, state_transitions);

        }
    }
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // после того как связный "каркас" сгенерирован, нужно случайным образом задать выходные переходы для каждого состояния
    for (auto &&qi : all_qi)
    {
        auto n_trans_out = dist_n_trans_out(gen);
        std::set<std::string> used_in_symbols;

        auto qi_opt = transitions.get_child_optional(qi);
        pt::ptree state_transitions;
        if (qi_opt) 
        {
            auto& qi_node = *qi_opt;
            for (auto&& zi : qi_node) 
            {
                used_in_symbols.insert(zi.first);
                state_transitions = qi_node;
            }
        }

        // от 1, а не от 0, т.к. в генерации связного "каркаса" уже есть по одному выходу на каждое состяние
        for (int j = 1; j < n_trans_out; ++j) 
        {
            std::string symb_in;
            while (used_in_symbols.find(symb_in) != used_in_symbols.end() || symb_in.empty() ) 
            {
                symb_in = "z" + std::to_string(dist_symb_in(gen));
            };
            used_in_symbols.insert(symb_in);

            std::string symb_out = "w" + std::to_string(dist_symb_out(gen));
            auto next_state = 1 + (gen() % n_states);

            pt::ptree transition;
            std::string added_state = "q" + std::to_string(next_state);
            transition.put("state", added_state);
            transition.put("output", symb_out);

            state_transitions.add_child(symb_in, transition);
        }
        transitions.put_child(qi, state_transitions);
    }

    tree.add_child("transitions", transitions);
    std::ofstream output(params.output_file);
    if (!output.is_open()) 
    {
        std::cerr << "Error opening output file!!!" << std::endl;
        return 2;
    }
    pt::write_json(output, tree);
    return 0;

}

int main(int argc, char* argv[]) {
    try 
    {
        po::options_description desc("Allowed options");
        desc.add_options()
            ("help", "produce help message")
            ("seed", po::value<int>()->required(), "set random seed")
            ("n_states_min", po::value<int>()->required(), "set minimum number of states")
            ("n_states_max", po::value<int>()->required(), "set maximum number of states")
            ("n_alph_in_min", po::value<int>()->required(), "set minimum input alphabet size")
            ("n_alph_in_max", po::value<int>()->required(), "set maximum input alphabet size")
            ("n_alph_out_min", po::value<int>()->required(), "set minimum output alphabet size")
            ("n_alph_out_max", po::value<int>()->required(), "set maximum output alphabet size")
            ("n_trans_out_min", po::value<int>()->required(), "set minimum number of outgoing transitions")
            ("n_trans_out_max", po::value<int>()->required(), "set maximum number of outgoing transitions")
            ("out", po::value<std::string>()->required(), "set output file name");

        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);

        if (vm.count("help")) 
        {
            std::cout << desc << "\n";
            return 0;
        }

        po::notify(vm);

        generator_parameters params;
        params.seed = vm["seed"].as<int>();
        params.n_states_min = vm["n_states_min"].as<int>();
        params.n_states_max = vm["n_states_max"].as<int>();
        params.n_alph_in_min = vm["n_alph_in_min"].as<int>();
        params.n_alph_in_max = vm["n_alph_in_max"].as<int>();
        params.n_alph_out_min = vm["n_alph_out_min"].as<int>();
        params.n_alph_out_max = vm["n_alph_out_max"].as<int>();
        params.n_trans_out_min = vm["n_trans_out_min"].as<int>();
        params.n_trans_out_max = vm["n_trans_out_max"].as<int>();
        params.output_file = vm["out"].as<std::string>();

        return generate_machine(params);

    } 
    catch (const po::error &e) 
    {
        std::cerr << "Command line error: " << e.what() << "\n";
        return 2;
    }
    catch (const std::exception &e) 
    {
        std::cerr << "Error: " << e.what() << "\n";
        return 2;
    }
}
