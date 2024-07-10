#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/program_options.hpp>
#include <random>
#include <iostream>
#include <fstream>
#include <set>
#include <vector>
#include <unordered_map>

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

std::vector<std::string> create_sequence_q_i(int n_states) {
    std::vector<std::string> sequence;
    for (int i = 1; i <= n_states; ++i) 
    {
        sequence.push_back("q" + std::to_string(i));
    }
    return sequence;
}

std::vector<std::string> missing_elements(const std::unordered_map<std::string, int>& container, const std::vector<std::string>& sequence) {
    std::vector<std::string> missing;
    for (const auto& elem : sequence) 
    {
        if (container.find(elem) == container.end()) 
        {
            missing.push_back(elem);
        }
    }
    return missing;
}

auto generate_machine(const generator_parameters &params) 
{
    // проверка введеных пользователем значений диапазонов
    if (params.n_states_min > params.n_states_max)
    {
        std::cerr << "incorrect range for states";
        return 2;
    }
    if (params.n_alph_in_min > params.n_alph_in_max)
    {
        std::cerr << "incorrect range for alph_in";
        return 2;
    }
    if (params.n_alph_out_min > params.n_alph_out_max)
    {
        std::cerr << "incorrect range for alph_out";
        return 2;
    }
    if (params.n_trans_out_min > params.n_trans_out_max)
    {
        std::cerr << "incorrect range for trans_out";
        return 2;
    }
    // если вдруг условие n_alph_in_max > n_trans_out_min нарушено, то детерминированного автомата вообще не может существовать
    if (params.n_alph_in_max < params.n_trans_out_min)
    {
        std::cerr << "incorrect combination between alph_in_max and trans_out_min - condition n_alph_in_max > n_trans_out_min is violated!!!";
        return 2;
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

    // для отслеживания состояний, в которые когда-либо вели переходы
    std::unordered_map<std::string, int> states_with_incoming_transitions;

    pt::ptree transitions;

    for (int i = 1; i <= n_states; ++i) 
    {

        std::string state_name = "q" + std::to_string(i);
        pt::ptree state_transitions;
        auto n_trans_out = dist_n_trans_out(gen);
        std::set<std::string> used_in_symbols;

        for (int j = 0; j < n_trans_out; ++j) {
            std::string symb_in;
            do 
            {

                symb_in = "z" + std::to_string(dist_symb_in(gen));

            } 
            while (used_in_symbols.find(symb_in) != used_in_symbols.end());
            used_in_symbols.insert(symb_in);

            std::string symb_out = "w" + std::to_string(dist_symb_out(gen));
            auto next_state = 1 + (gen() % n_states);

            pt::ptree transition;
            std::string added_state = "q" + std::to_string(next_state);
            transition.put("state", added_state);

            auto it = states_with_incoming_transitions.find(added_state);
            if (it == states_with_incoming_transitions.end()) 
            {
                states_with_incoming_transitions.insert({added_state, 1});
            } 
            else 
            {
                it->second++;
            }
            
            transition.put("output", symb_out);
            state_transitions.add_child(symb_in, transition);
        }
        transitions.add_child(state_name, state_transitions);
    }

 
    auto all_states = create_sequence_q_i(n_states);
    auto without_in_transitions = missing_elements(states_with_incoming_transitions, all_states);

    // цикл для достижения того, что в каждое состояние вел хотя бы один переход
    auto ind = 0;
    while(ind < without_in_transitions.size()) {
        bool transition_made = false;

        for (auto &&i : all_states)
        {
            auto qi_opt = transitions.get_child_optional(i);
            if (!qi_opt) continue;

            auto& qi = *qi_opt;
            auto z_count = qi.size();
                
            // если число исходящих переходов > 1, то один из них, вероятно, можно перенаправить в то состояние, в которого не входит не один переход
            if (z_count > 1) 
            {
                // перенаправлять имеем право только тот, который не является единственным входящим в какое-дибо другое состояние переход
                for (auto&& zi : qi) 
                {
                    auto state_opt = zi.second.get_optional<std::string>("state");
                    if (!state_opt) continue;

                    auto it = states_with_incoming_transitions.find(*state_opt);
                    if (it->second > 1) {
            
                        zi.second.put("state", without_in_transitions[ind]);
                        it->second--;
                        ind++;
                        transition_made = true;
                        if (ind >= without_in_transitions.size()) 
                        {
                            break;
                        }
                    }
                }
            }
            if (ind >= without_in_transitions.size()) 
            {
            break;
            }
        }
        // возможен случай, когда в состояния ведут по одному и менее переходов, тогда ни при каких условиях не получится перекинуть хотя бы один переход из них в состояние с 0 входных переходов
        if (!transition_made) 
        {
            std::cerr << "No suitable transition found to redirect, exiting to avoid infinite loop." << std::endl;
            break;
        }
    }

    tree.add_child("transitions", transitions);

    std::ofstream output(params.output_file);
    if (!output.is_open()) 
    {
        std::cerr << "Error opening output file!!!" << std::endl;
        return 1;
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
            ("seed", po::value<int>(), "set random seed")
            ("n_states_min", po::value<int>(), "set minimum number of states")
            ("n_states_max", po::value<int>(), "set maximum number of states")
            ("n_alph_in_min", po::value<int>(), "set minimum input alphabet size")
            ("n_alph_in_max", po::value<int>(), "set maximum input alphabet size")
            ("n_alph_out_min", po::value<int>(), "set minimum output alphabet size")
            ("n_alph_out_max", po::value<int>(), "set maximum output alphabet size")
            ("n_trans_out_min", po::value<int>(), "set minimum number of outgoing transitions")
            ("n_trans_out_max", po::value<int>(), "set maximum number of outgoing transitions")
            ("out", po::value<std::string>(), "set output file name");

        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);

        if (vm.count("help")) {
            std::cout << desc << "\n";
            return 1;
        }

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

        generate_machine(params);

    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
