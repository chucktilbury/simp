/**
 * @file PathResolution.hpp
 * @brief Relocatable resource and module search path resolution.
 *
 * Compiler resources are located relative to the running executable using the
 * same relative layout as an installed tree, so the compiler never depends on
 * absolute source, build, or install paths captured at build time.
 */
#pragma once

#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace simp {

/// Install directories relative to the installation prefix.
struct InstallLayout {
    std::filesystem::path binDirectory;
    std::filesystem::path libDirectory;
    std::filesystem::path includeDirectory;
    std::filesystem::path dataDirectory;
};

/// A resolved path together with a human-readable description of its source.
struct ResolvedPath {
    std::filesystem::path path;
    std::string origin;
};

/// Returns a non-empty environment value, or nothing when unset or empty.
using EnvironmentLookup = std::function<std::optional<std::string>(const std::string&)>;

struct ResourcePaths {
    std::optional<std::filesystem::path> executable;
    std::optional<ResolvedPath> prefix;
    ResolvedPath runtimeDirectory;
    std::filesystem::path runtimeLibrary;
    ResolvedPath includeDirectory;
    ResolvedPath preludeDirectory;
    std::filesystem::path preludeSource;
    ResolvedPath standardModuleDirectory;
};

struct ModuleSearchRequest {
    std::optional<std::string> moduleDirectoryOption;
    std::vector<std::string> packagePathOptions;
    std::vector<std::string> sourcePaths;
    std::filesystem::path currentDirectory;
};

struct ModuleSearchPaths {
    ResolvedPath projectRoot;
    ResolvedPath projectModuleRoot;
    ResolvedPath moduleSelectionFile;
    bool projectModuleRootExplicit = false;
    ResolvedPath standardModuleRoot;
    std::vector<ResolvedPath> compatibilityRoots;
    ResolvedPath registry;
    std::vector<std::string> deprecationWarnings;

    /// Package roots in search order: project, standard, then compatibility.
    std::vector<ResolvedPath> packageRoots() const;
};

/// The relative install layout this compiler was configured with.
InstallLayout configuredInstallLayout();

/// Reads the process environment, treating empty values as unset.
std::optional<std::string> processEnvironment(const std::string& name);

/// Locates the running executable (Linux /proc, BSD sysctl, then argv[0]/PATH).
std::optional<std::filesystem::path> currentExecutablePath(const std::string& argv0);

/// Derives the installation prefix from an executable inside layout.binDirectory.
std::filesystem::path installPrefixForExecutable(const std::filesystem::path& executable,
                                                 const InstallLayout& layout);

/**
 * Resolves compiler resources. Resource-specific variables (SIMP_RUNTIME_DIR,
 * SIMP_INCLUDE_DIR, SIMP_PRELUDE_DIR, SIMP_STDLIB_MODULE_DIR) take precedence,
 * then SIMP_HOME as an installation prefix, then the executable-relative prefix.
 * Throws std::runtime_error when a resource needs the executable-relative
 * prefix but the executable location is unknown.
 */
ResourcePaths resolveResourcePaths(const std::optional<std::filesystem::path>& executable,
                                   const InstallLayout& layout,
                                   const EnvironmentLookup& environment);

/// Resources for the current process, resolved once without argv[0].
const ResourcePaths& processResourcePaths();

/**
 * Resolves the module search configuration. The project module root comes from
 * -M/--module-dir, then SIMP_MODULE_DIR, then <project-root>/modules where the
 * project root is the nearest ancestor of the first source input containing
 * simpkg.toml or modules (starting at the current directory without sources).
 * With no marker, the source parent/current directory is used. Locked projects
 * use simpkg.lock; simultaneous legacy and new policies are errors.
 */
ModuleSearchPaths resolveModuleSearchPaths(const ModuleSearchRequest& request,
                                           const ResourcePaths& resources,
                                           const EnvironmentLookup& environment);

/// Throws std::runtime_error when an explicitly selected module root is missing.
void validateModuleSearchPaths(const ModuleSearchPaths& paths);

} // namespace simp
