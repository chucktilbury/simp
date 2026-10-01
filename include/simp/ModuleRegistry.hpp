#pragma once

#include "simp/Ast.hpp"
#include "simp/PathResolution.hpp"

#include <filesystem>
#include <string>
#include <vector>

namespace simp {

struct ModuleLoadOptions {
    /// Package manifest roots in search order.
    std::vector<ResolvedPath> packageSearchRoots;
    /// Deprecated tab-separated registry consulted after all manifest roots.
    ResolvedPath registry;
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
    std::vector<std::string> warnings;
};

ModuleLoadResult loadImportedModules(Program& program,
                                     const ModuleLoadOptions& options);

} // namespace simp
