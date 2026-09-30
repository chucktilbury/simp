#pragma once

#include "simp/Ast.hpp"

#include <filesystem>
#include <string>
#include <vector>

namespace simp {

struct ModuleLoadOptions {
    std::filesystem::path registryPath;
    std::vector<std::filesystem::path> packageSearchRoots;
};

struct LoadedModule {
    std::string name;
    std::string version;
    std::vector<std::string> dependencyVersions;
    std::filesystem::path sourcePath;
};

struct ModuleLoadResult {
    std::vector<LoadedModule> modules;
    std::vector<std::filesystem::path> libraryPaths;
    std::vector<std::string> libraries;
};

ModuleLoadResult loadImportedModules(Program& program,
                                     const ModuleLoadOptions& options);

} // namespace simp
