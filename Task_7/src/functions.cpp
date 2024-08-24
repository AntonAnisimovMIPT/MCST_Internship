#include "functions.hpp"

Generator::Generator(const std::string& config_file, unsigned int seed)
    : gen(seed) {
    cache_config = load_cache_config(config_file);
}

CacheConfig Generator::load_cache_config(const std::string& config_file) {
    CacheConfig config;
    pt::ptree root;
    pt::read_json(config_file, root);

    config.line_size = root.get<unsigned int>("cache_descr.line_size");
    config.associativity = root.get<unsigned int>("cache_descr.associativity");
    config.cache_size = root.get<unsigned int>("cache_descr.cache_size");

    return config;
}

std::string Generator::generate_operation(double prob) {
    std::uniform_real_distribution<> dist(0.0, 1.0);
    auto is_write_operation = (dist(gen) < prob);
    auto addr = generate_address();
    std::uint32_t data;

    std::ostringstream oss;
    if (is_write_operation) {
        data = generate_data();
        history[addr] = data;
        oss << 'W' << " 0x" << std::hex << std::setw(8) << std::setfill('0') << addr
            << " 0x" << std::hex << std::setw(8) << std::setfill('0') << data;
    } else {
        auto it = history.find(addr);
        if (it != history.end()) {
            data = it->second;
        } else {
            data = generate_data();
            history[addr] = data;
        }
        oss << 'R' << " 0x" << std::hex << std::setw(8) << std::setfill('0') << addr
            << " 0x" << std::hex << std::setw(8) << std::setfill('0') << data;
    }

    return oss.str();
}

std::uint32_t Generator::generate_address() {
    std::uniform_int_distribution<std::uint32_t> dist(0, cache_config.cache_size - cache_config.line_size);
    auto addr = dist(gen);
    return addr & ~(cache_config.line_size - 1);
}

std::uint32_t Generator::generate_data() {
    std::uniform_int_distribution<std::uint32_t> dist(0, 0xFFFFFFFF);
    return dist(gen);
}

void Generator::generate_tests(const std::string& output_file, unsigned int num_operations, double read_prob) {
    std::ofstream ofs(output_file);
    if (!ofs) {
        throw std::runtime_error("Failed to open output file: " + output_file);
    }

    for (int i = 0; i < num_operations; ++i) {
        ofs << generate_operation(read_prob) << std::endl;
    }
}
