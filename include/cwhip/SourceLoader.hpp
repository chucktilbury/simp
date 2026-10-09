#pragma once

#include "cwhip/Token.hpp"

#include <cstddef>
#include <filesystem>
#include <string>
#include <unordered_set>
#include <vector>

namespace cwhip {

std::vector<Token> tokenizeWithIncludes(
    const std::string& source, const std::filesystem::path& sourcePath,
    std::unordered_set<std::string>& includedFiles, std::size_t depth, bool root,
    std::size_t maximumIncludeDepth = 16,
    std::vector<std::filesystem::path> includeChain = {},
    std::vector<std::filesystem::path> includeSearchPaths = {});

} // namespace cwhip
