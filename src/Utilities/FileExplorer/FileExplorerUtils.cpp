#include "FileExplorerUtils.h"

#include <vector>
#include <sstream>

std::string FileExplorerUtils::getFileExtension(const std::string& filename) {
    std::stringstream ss(filename);
    std::string item;
    std::vector<std::string> result;

    // Split by '.'
    while (std::getline(ss, item, '.')) {
        result.push_back(item);
    }

    return result.back();
}