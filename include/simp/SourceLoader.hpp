#pragma once

#include "simp/Token.hpp"

#include <cstddef>
#include <filesystem>
#include <string>
#include <unordered_set>
#include <vector>

namespace simp {

std::vector<Token> tokenizeWithIncludes(
    const std::string& source, const std::filesystem::path& sourcePath,
    std::unordered_set<std::string>& includedFiles, std::size_t depth, bool root,
    std::size_t maximumIncludeDepth = 16,
    std::vector<std::filesystem::path> includeChain = {});

} // namespace simp
