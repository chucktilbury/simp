#pragma once

#include "cwhip/Ast.hpp"
#include "cwhip/PathResolution.hpp"

#include <filesystem>
#include <string>
#include <vector>

namespace cwhip {

struct ModuleLoadOptions {
    /// Package manifest roots in search order.
    std::vector<ResolvedPath> packageSearchRoots;
    /// Project cwhip-pkg.lock; package storage roots never relocate this path.
    std::filesystem::path projectLockFile;
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
    bool versionPolicyActive = false;
};

ModuleLoadResult loadImportedModules(Program& program,
                                     const ModuleLoadOptions& options);

} // namespace cwhip
