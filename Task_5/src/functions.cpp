#include "functions.hpp"

void save_json_to_file(const pt::ptree& json, const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return;
    }
    pt::write_json(file, json);
    file.close();
}

std::string vector_to_string(const std::vector<int>& vec) {
    std::ostringstream oss;

    for (size_t i = 0; i < vec.size(); ++i) {
        oss << vec[i];
        if (i < vec.size() - 1) {
            oss << ",";
        }
    }

    return oss.str();
}
