#include "functions.hpp"

Generator::Generator(const std::string& config_file, unsigned int seed)
    : gen(seed),
      tag_dist(0, 0xFFFFFFF),
      prob_dist(0.0, 1.0),
      first_operation(true) {
    cache_config = load_cache_config(config_file);
    set_dist = std::uniform_int_distribution<unsigned int>(0, (cache_config.cache_size / cache_config.line_size) - 1);
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
    std::uint32_t addr;
    std::uint32_t data;

    std::ostringstream oss;

    if (first_operation) {
        // первая операция - всегда запись
        addr = generate_address();
        data = generate_data();
        history[addr] = data;
        oss << 'W' << " 0x" << std::hex << std::setw(8) << std::setfill('0') << addr
            << " 0x" << std::hex << std::setw(8) << std::setfill('0') << data;
        first_operation = false;
    } else {
        auto is_write_operation = prob_dist(gen) < prob;

        if (is_write_operation) {
            addr = generate_address();
            data = generate_data();
            history[addr] = data;
            oss << 'W' << " 0x" << std::hex << std::setw(8) << std::setfill('0') << addr
                << " 0x" << std::hex << std::setw(8) << std::setfill('0') << data;
        } else {
            addr = get_random_initialized_address();
            data = history[addr];
            oss << 'R' << " 0x" << std::hex << std::setw(8) << std::setfill('0') << addr
                << " 0x" << std::hex << std::setw(8) << std::setfill('0') << data;
        }
    }

    return oss.str();
}

std::uint32_t Generator::generate_address() {
    // проверка наличия заполненных наборов
    std::unordered_map<unsigned int, std::vector<std::uint32_t>> set_addresses;
    for (const auto& [addr, data] : history) {
        unsigned int set_index = (addr / cache_config.line_size) % (cache_config.cache_size / cache_config.line_size);
        set_addresses[set_index].push_back(addr);
    }

    if (!set_addresses.empty()) {
        // случайно выбираем заполненный набор
        std::vector<unsigned int> filled_sets;
        for (const auto& [set_index, _] : set_addresses) {
            filled_sets.push_back(set_index);
        }
        std::uniform_int_distribution<> set_dist(0, filled_sets.size() - 1);
        unsigned int set_index = filled_sets[set_dist(gen)];

        // генерируем уникальный адрес для данного набора, который сделает вытеснение
        return generate_unique_address_for_set(set_index, history);
    } else {
        // если заполненных нет, то случайный адрес
        return set_dist(gen) * cache_config.line_size;
    }
}

std::uint32_t Generator::generate_unique_address_for_set(unsigned int set_index, std::unordered_map<std::uint32_t, std::uint32_t>& history) {
    // вычисление базового адреса для набора
    std::uint32_t base_addr = set_index * cache_config.line_size;

    unsigned int line_size_bits = std::log2(cache_config.line_size);
    unsigned int set_count_bits = std::log2(cache_config.cache_size / (cache_config.line_size * cache_config.associativity));

    // убеждаемся, что адрес уникальный и вызывает вытеснение
    std::uint32_t tag = tag_dist(gen);
    std::uint32_t address = base_addr | (tag << (line_size_bits + set_count_bits));

    while (history.find(address) != history.end()) {
        tag = tag_dist(gen);
        address = base_addr | (tag << (line_size_bits + set_count_bits));
    }

    return address;
}

std::uint32_t Generator::generate_data() {
    return tag_dist(gen); // т.к. tag тоже генерирует от 0 до 0xFFFFFFF
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

std::uint32_t Generator::get_random_initialized_address() {
    if (history.empty()) {
        throw std::runtime_error("No initialized addresses available for reading.");
    }
    std::uniform_int_distribution<> index_dist(0, history.size() - 1);
    auto it = history.begin();
    std::advance(it, index_dist(gen));
    return it->first;
}
