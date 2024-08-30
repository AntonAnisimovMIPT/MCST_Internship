#ifndef FUNCTIONS_HPP
#define FUNCTIONS_HPP

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include <fstream>
#include <iomanip>
#include <iostream>

#include <random>

#include <vector>
#include <string>
#include <unordered_map>

namespace pt = boost::property_tree;

struct CacheConfig {
    unsigned int line_size;
    unsigned int associativity;
    unsigned int cache_size;
};

class Generator {
  public:
    Generator(const std::string& config_file, unsigned int seed);
    void generate_tests(const std::string& output_file, unsigned int num_operations, double prob);

  private:
    CacheConfig cache_config;
    std::mt19937 gen;
    std::uniform_int_distribution<std::uint32_t> set_dist;
    std::uniform_int_distribution<std::uint32_t> tag_dist;
    std::uniform_real_distribution<double> prob_dist;

    CacheConfig load_cache_config(const std::string& config_file);
    std::string generate_operation(double prob);
    std::uint32_t generate_address();
    std::uint32_t generate_data();
    std::uint32_t get_random_initialized_address();
    std::uint32_t generate_unique_address_for_set(unsigned int set_index, std::unordered_map<std::uint32_t, std::uint32_t>& history);

    std::unordered_map<std::uint32_t, std::uint32_t> history;

    bool first_operation;
};

#endif