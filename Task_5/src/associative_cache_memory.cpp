#include "functions.hpp"

enum class State {
    INACTION,
    SEARCHING,
    ADDING,
    OVERWRITING
};

enum class Output {
    DEFAULT,
    FOUND,
    NOT_FOUND,
    ADDED,
    OVERWRITTEN
};

const std::vector<std::string> state_names = {
    "INACTION",
    "SEARCHING",
    "ADDING",
    "OVERWRITING"};

const std::vector<std::string> output_names = {
    "FOUND",
    "NOT_FOUND",
    "ADDED",
    "OVERWRITTEN"};

class Cache {
  public:
    Cache(int cache_size, int block_size, pt::ptree& log_tree)
        : cache_size_(cache_size), block_size_(block_size), state(State::INACTION), log_tree_(log_tree) {
        cache_lines_.resize(cache_size, CacheLine(block_size));
    }

    void search(int tag) {
        Transition tr;
        tr.current_state = "SEARCHING";
        tr.next_state = "INACTION";
        tr.input = std::to_string(tag);

        for (const auto& line : cache_lines_) {
            if (line.valid && line.tag == tag) {
                tr.output = "FOUND";
                add_transition_to_log(tr);
                return;
            }
        }
        tr.output = "NOT_FOUND";
        add_transition_to_log(tr);
    }

    void add(int tag, const std::vector<int>& data_block) {
        Transition tr;
        tr.current_state = "ADDING";
        tr.next_state = "INACTION";
        tr.input = std::to_string(tag) + ":" + vector_to_string(data_block);

        for (auto& line : cache_lines_) {
            if (!line.valid) {
                line.tag = tag;
                line.data_block = data_block;
                line.valid = true;

                tr.output = "ADDED";
                add_transition_to_log(tr);

                return;
            }
        }
        // Если нет свободного места, выбрасываем исключение
        std::string msg = "all cache lines are busy, adding is not possible!!!";
        throw std::runtime_error(msg);
    }

    void overwrite_LRU(int tag, const std::vector<int>& data_block) {
        Transition tr;
        tr.current_state = "OVERWRITING";
        tr.next_state = "INACTION";
        tr.input = std::to_string(tag) + ":" + vector_to_string(data_block);
        tr.output = "OVERWRITTEN";

        add_transition_to_log(tr);
        cache_lines_[0].tag = tag;
        cache_lines_[0].data_block = data_block;
        cache_lines_[0].valid = true;
    }

    pt::ptree get_json() const {
        return log_tree_;
    }

  private:
    int cache_size_;
    int block_size_;
    std::vector<CacheLine> cache_lines_;
    State state;
    pt::ptree& log_tree_;

    void add_transition_to_log(const Transition& tr) {
        // Проверка и добавление начального состояния
        if (log_tree_.empty()) {
            log_tree_.put("initial_state", tr.current_state);
        }

        // Попытка найти существующий узел для текущего состояния
        auto state_node_opt = log_tree_.get_child_optional("transitions." + tr.current_state);

        pt::ptree state_node;
        if (state_node_opt) {
            state_node = *state_node_opt;
        } else {
            // Если узел не найден, создаем новый
            log_tree_.put_child("transitions." + tr.current_state, state_node);
        }

        // Проверяем, есть ли уже поддерево для input символа
        auto input_node_opt = state_node.get_child_optional(tr.input);

        pt::ptree input_node;
        if (input_node_opt) {
            input_node = *input_node_opt;
        } else {
            // Если поддерево для символа ввода не найдено, создаем новое
            state_node.put_child(tr.input, input_node);
        }

        // Обновляем информацию о next state и output
        input_node.put("state", tr.next_state);
        input_node.put("output", tr.output);

        // Сохраняем обновленные данные обратно в дерево
        state_node.put_child(tr.input, input_node);
        log_tree_.put_child("transitions." + tr.current_state, state_node);
    }
};

int main() {
    int cache_size = 4;
    int block_size = 4;
    pt::ptree log_tree;
    Cache cache(cache_size, block_size, log_tree);

    std::vector<std::pair<int, std::vector<int>>> operations = {
        {0x123, {1, 2, 3, 4}},
        {0xABC, {5, 6, 7, 8}},
        {0x456, {9, 10, 11, 12}},
        {0xDEF, {13, 14, 15, 16}},
        {0xABC, {17, 18, 19, 20}}};

    cache.add(operations[0].first, operations[0].second);
    cache.search(0x123);
    cache.search(0xABC);
    cache.add(operations[1].first, operations[1].second);
    cache.search(0xABC);
    cache.add(operations[2].first, operations[2].second);
    cache.add(operations[3].first, operations[3].second);
    // cache.add(operations[4].first, operations[4].second);

    // Сохранение логов операций в JSON-файл
    save_json_to_file(cache.get_json(), "../cache_emulations.json");

    return 0;
}