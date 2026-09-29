#pragma once

#include "simp/Ast.hpp"

#include <filesystem>
#include <string>
#include <vector>

namespace simp {

struct LoadedModule {
    std::string name;
    std::string version;
    std::vector<std::string> dependencyVersions;
    std::filesystem::path sourcePath;
};

std::vector<LoadedModule> loadImportedModules(
    Program& program, const std::filesystem::path& registryPath);

} // namespace simp
