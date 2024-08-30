#include "functions.hpp"

CacheLine::CacheLine()
    : tag(0), data(0), last_used(0), valid(false), modif(false), initialized(false) {}

CacheSet::CacheSet(int associativity) {
    lines.resize(associativity);
}

int CacheSet::find_line(std::uint32_t tag) {
    for (int i = 0; i < lines.size(); ++i) {
        if (lines[i].valid && lines[i].tag == tag) {
            return i;
        }
    }
    return -1;
}

int CacheSet::find_victim() {
    auto LRU_index = 0;
    for (int i = 1; i < lines.size(); ++i) {
        if (lines[i].last_used < lines[LRU_index].last_used) {
            LRU_index = i;
        }
    }
    return LRU_index;
}

CacheLine& CacheSet::get_line(int index) {
    return lines[index];
}

void CacheSet::set_line(int index, const CacheLine& line) {
    lines[index] = line;
}

Cache::Cache(int cache_size, int line_size, int associativity)
    : line_size(line_size), associativity(associativity), access_counter(0) {
    num_sets = cache_size / (line_size * associativity);
    sets.resize(num_sets, CacheSet(associativity));
    index_bits = static_cast<int>(std::log2(num_sets));
    offset_bits = static_cast<int>(std::log2(line_size));
    tag_bits = 32 - index_bits - offset_bits;
}

std::uint32_t Cache::get_index(std::uint32_t addr) {
    return (addr >> offset_bits) & ((1 << index_bits) - 1);
}

std::uint32_t Cache::get_tag(uint32_t addr) {
    return addr >> (offset_bits + index_bits);
}

std::uint32_t Cache::get_way(std::uint32_t addr) {
    auto index = get_index(addr);
    auto tag = get_tag(addr);

    auto way = sets[index].find_line(tag);

    if (way == -1) {
        way = sets[index].find_victim();
    }

    return way;
}

void Cache::read(uint32_t addr, uint32_t expected_data) {
    auto index = get_index(addr);
    auto tag = get_tag(addr);
    auto way = get_way(addr);
    access_counter++;

    std::cout << "Read from addr: 0x"
              << std::hex << std::setw(8) << std::setfill('0') << addr << std::endl;

    auto& line = sets[index].get_line(way);
    if (!line.initialized) {
        std::cerr << "Error: Attempt to read uninitialized data at address: 0x"
                  << std::hex << std::setw(8) << std::setfill('0') << addr << std::endl;
        exit(1);
    }

    if (line.valid && line.tag == tag) {

        line.last_used = access_counter;

        std::cout << "Cache hit: tag=0x"
                  << std::hex << std::setw(8) << std::setfill('0') << tag
                  << " index="
                  << std::dec << index
                  << " way="
                  << std::dec << way << std::endl;

        if (line.data != expected_data) {
            std::cerr << "Data mismatch! Expected: 0x"
                      << std::hex << std::setw(8) << std::setfill('0') << expected_data
                      << ", Found: 0x"
                      << std::hex << std::setw(8) << std::setfill('0') << line.data << std::endl;
            exit(1);
        }

        std::cout << "Data returned to core: 0x"
                  << std::hex << std::setw(8) << std::setfill('0') << line.data << std::endl;

        std::cout << "----------\n";
    } else {
        if (line.valid) {
            auto evicted_addr = (line.tag << (index_bits + offset_bits)) | (index << offset_bits);
            if (line.modif) {
                std::cout << "Evicting modified line: "
                          << "address=0x"
                          << std::hex << std::setw(8) << std::setfill('0') << evicted_addr
                          << " tag=0x"
                          << std::hex << std::setw(8) << std::setfill('0') << line.tag
                          << " index="
                          << std::dec << index
                          << " way="
                          << std::dec << way
                          << " data=0x"
                          << std::hex << std::setw(8) << std::setfill('0') << line.data << std::endl;

            } else {
                std::cout << "Evicting unmodified line: "
                          << "tag=0x"
                          << std::hex << std::setw(8) << std::setfill('0') << line.tag
                          << " index="
                          << std::dec << index
                          << " way="
                          << way << std::endl;
            }
        }

        std::mt19937 generator(std::random_device{}());
        std::uniform_int_distribution<std::uint32_t> distribution(0, 0xFFFFFFFF);

        auto memory_data = distribution(generator);
        std::cout << "RAM read: address=0x"
                  << std::hex << std::setw(8) << std::setfill('0') << addr
                  << " data=0x"
                  << std::hex << std::setw(8) << std::setfill('0') << memory_data << std::endl;

        line.tag = tag;
        line.data = expected_data;
        line.initialized = true;
        line.valid = true;
        line.modif = false;
        line.last_used = access_counter;

        std::cout << "Stored in cache: tag=0x"
                  << std::hex << std::setw(8) << std::setfill('0') << tag
                  << " index="
                  << std::dec << index
                  << " way=" << std::dec << way << std::endl;

        std::cout << "Data returned to core: 0x"
                  << std::hex << std::setw(8) << std::setfill('0') << memory_data << std::endl;

        std::cout << "----------\n";
    }
}

void Cache::write(uint32_t addr, uint32_t data) {
    auto index = get_index(addr);
    auto tag = get_tag(addr);
    access_counter++;

    std::cout << "Write to addr: 0x"
              << std::hex << std::setw(8) << std::setfill('0') << addr
              << " data=0x"
              << std::hex << std::setw(8) << std::setfill('0') << data << std::endl;

    // существует ли уже строка с этим tag в наборе
    auto way = sets[index].find_line(tag);
    if (way != -1) {
        // если строка с этим tag найдена
        auto& line = sets[index].get_line(way);

        line.data = data;
        line.modif = true;
        line.initialized = true;
        line.last_used = access_counter;

        std::cout << "Cache hit - updated existing line: tag=0x"
                  << std::hex << std::setw(8) << std::setfill('0') << tag
                  << " index="
                  << std::dec << index
                  << " way="
                  << std::dec << way << std::endl;

        std::cout << "----------\n";
    } else {
        // tag не найден, значит производим вытеснение
        way = sets[index].find_victim();
        auto& evicted_line = sets[index].get_line(way);

        if (evicted_line.valid) {
            auto evicted_addr = (evicted_line.tag << (index_bits + offset_bits)) | (index << offset_bits);
            if (evicted_line.modif) {
                std::cout << "Evicting modified line: "
                          << "address=0x"
                          << std::hex << std::setw(8) << std::setfill('0') << evicted_addr
                          << " tag=0x"
                          << std::hex << std::setw(8) << std::setfill('0') << evicted_line.tag
                          << " index="
                          << std::dec << index
                          << " way="
                          << std::dec << way
                          << " data=0x"
                          << std::hex << std::setw(8) << std::setfill('0') << evicted_line.data << std::endl;

                std::cout << "Writing back to RAM: address=0x"
                          << std::hex << std::setw(8) << std::setfill('0') << evicted_addr
                          << " data=0x"
                          << std::hex << std::setfill('0') << evicted_line.data << std::endl;
            } else {
                std::cout << "Evicting unmodified line: "
                          << "tag=0x"
                          << std::hex << std::setw(8) << std::setfill('0') << evicted_line.tag
                          << " index="
                          << std::dec << index
                          << " way="
                          << std::dec << way << std::endl;
            }
        }

        evicted_line.tag = tag;
        evicted_line.data = data;
        evicted_line.initialized = true;
        evicted_line.valid = true;
        evicted_line.modif = true;
        evicted_line.last_used = access_counter;

        std::cout << "Stored in cache: tag=0x"
                  << std::hex << std::setw(8) << std::setfill('0') << tag
                  << " index="
                  << std::dec << index
                  << " way="
                  << std::dec << way << std::endl;

        std::cout << "----------\n";
    }
}
