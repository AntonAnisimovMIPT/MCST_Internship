#ifndef FUNCTIONS_HPP
#define FUNCTIONS_HPP

#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/program_options.hpp>

#include <iostream>
#include <iomanip>
#include <fstream>

#include <vector>

#include <cstdint>
#include <stdexcept>

namespace pt = boost::property_tree;
namespace po = boost::program_options;

struct CacheLine {
    std::uint32_t tag;
    std::uint32_t data;
    std::uint32_t last_used;
    bool valid;
    bool modif;

    CacheLine();
};

class CacheSet {
  public:
    CacheSet(int associativity);

    int find_line(std::uint32_t tag);
    int find_victim();

    CacheLine& get_line(int index);
    void set_line(int index, const CacheLine& line);

  private:
    std::vector<CacheLine> lines;
};

class Cache {
  public:
    Cache(int cache_size, int line_size, int associativity);

    void read(std::uint32_t addr, std::uint32_t expected_data);
    void write(std::uint32_t addr, std::uint32_t data);

  private:
    std::vector<CacheSet> sets;
    int line_size;
    int associativity;
    int num_sets;

    int index_bits;
    int offset_bits;
    int tag_bits;

    std::uint32_t access_counter;
    std::uint32_t get_index(std::uint32_t addr);
    std::uint32_t get_tag(std::uint32_t addr);
    std::uint32_t get_way(std::uint32_t addr);
};

#endif