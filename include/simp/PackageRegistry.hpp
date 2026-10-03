#pragma once

#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace simp {

struct PackageDependency {
    std::string name;
    std::string version;
};

struct ResolvedPackage {
    std::string name;
    std::string version;
    std::filesystem::path packageRoot;
    std::filesystem::path sourcePath;
    std::string exportKind;
    std::string exportName;
    std::vector<PackageDependency> dependencies;
    std::vector<std::string> libraries;
    std::vector<std::filesystem::path> libraryPaths;
};

struct PackageResolution {
    std::unordered_map<std::string, ResolvedPackage> packages;
    std::vector<ResolvedPackage> linkOrder;
};

struct ModuleVersionPolicy {
    std::filesystem::path path;
    std::map<std::string, std::vector<std::string>> versions;
};

/// Returns no policy when the file does not exist; malformed files are errors.
std::optional<ModuleVersionPolicy> readModuleVersionPolicy(
    const std::filesystem::path& path);

PackageResolution resolvePackages(
    const std::vector<std::string>& rootNames,
    const std::vector<std::filesystem::path>& searchRoots,
    const std::optional<ModuleVersionPolicy>& policy = std::nullopt,
    const std::unordered_set<std::string>& legacyRegistryModules = {});

} // namespace simp
