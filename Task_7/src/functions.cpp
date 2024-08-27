#include "functions.hpp"

Generator::Generator(const std::string& config_file, unsigned int seed)
    : gen(seed),
      set_dist(0, (cache_config.cache_size / cache_config.line_size) - 1),
      tag_dist(0, 0xFFFFFFF),
      prob_dist(0.0, 1.0),
      first_operation(true) {
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
    std::uint32_t addr;
    std::uint32_t data;

    std::ostringstream oss;

    if (first_operation) {
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
    // Генерируем адреса так, чтобы они попадали в один и тот же набор
    auto set_index = set_dist(gen);

    // Генерируем разные теги для проверки замещения
    auto tag = tag_dist(gen);

    // Составляем адрес, который попадает в выбранный набор
    return (set_index * cache_config.line_size) | (tag << (cache_config.line_size * 4));
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
    // для вариативности, такое тоже возможно
    if (history.empty()) {
        exit(1);
    }
    std::uniform_int_distribution<> index_dist(0, history.size() - 1);
    auto it = history.begin();
    std::advance(it, index_dist(gen));
    return it->first;
}
