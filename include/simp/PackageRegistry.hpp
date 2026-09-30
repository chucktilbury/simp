#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>
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

PackageResolution resolvePackages(
    const std::vector<std::string>& rootNames,
    const std::vector<std::filesystem::path>& searchRoots);

} // namespace simp
