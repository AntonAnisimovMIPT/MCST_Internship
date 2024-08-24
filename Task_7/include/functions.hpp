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

    CacheConfig load_cache_config(const std::string& config_file);
    std::string generate_operation(double prob);
    std::uint32_t generate_address();
    std::uint32_t generate_data();

    std::unordered_map<std::uint32_t, std::uint32_t> history;
    std::unordered_map<std::uint32_t, bool> valid_entries;
};

#endif