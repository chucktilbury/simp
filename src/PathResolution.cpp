/**
 * @file PathResolution.cpp
 * @brief Relocatable resource and module search path resolution.
 */
#include "simp/PathResolution.hpp"

#include <cstdlib>
#include <set>
#include <stdexcept>
#include <system_error>
#include <utility>

#include <climits>
#include <unistd.h>

#if defined(__FreeBSD__) || defined(__DragonFly__) || defined(__NetBSD__) || \
    defined(__OpenBSD__)
#include <sys/types.h>
#include <sys/sysctl.h>
#endif

#ifndef SIMP_INSTALL_BINDIR
#error "SIMP_INSTALL_BINDIR must name the relative executable directory"
#endif
#ifndef SIMP_INSTALL_LIBDIR
#error "SIMP_INSTALL_LIBDIR must name the relative library directory"
#endif
#ifndef SIMP_INSTALL_INCLUDEDIR
#error "SIMP_INSTALL_INCLUDEDIR must name the relative header directory"
#endif
#ifndef SIMP_INSTALL_DATADIR
#error "SIMP_INSTALL_DATADIR must name the relative data directory"
#endif
#ifndef SIMP_RUNTIME_LIBRARY_NAME
#error "SIMP_RUNTIME_LIBRARY_NAME must name the runtime library file"
#endif

namespace simp {
namespace {

constexpr const char* preludeFileName = "String.simp";

std::filesystem::path normalizedAbsolute(const std::filesystem::path& path) {
    return std::filesystem::absolute(path).lexically_normal();
}

std::optional<std::filesystem::path> canonicalExecutable(const std::filesystem::path& path) {
    std::error_code error;
    if (!std::filesystem::is_regular_file(path, error)) return std::nullopt;
    if (access(path.c_str(), X_OK) != 0) return std::nullopt;
    auto canonical = std::filesystem::canonical(path, error);
    if (error) return std::nullopt;
    return canonical;
}

std::optional<std::filesystem::path> platformExecutablePath() {
#if defined(__linux__)
    std::error_code error;
    const auto target = std::filesystem::read_symlink("/proc/self/exe", error);
    if (!error && target.is_absolute()) return canonicalExecutable(target);
#elif defined(__FreeBSD__) || defined(__DragonFly__) || defined(__NetBSD__)
#if defined(__NetBSD__)
    int name[] = {CTL_KERN, KERN_PROC_ARGS, -1, KERN_PROC_PATHNAME};
#else
    int name[] = {CTL_KERN, KERN_PROC, KERN_PROC_PATHNAME, -1};
#endif
    char buffer[PATH_MAX];
    std::size_t length = sizeof(buffer);
    if (sysctl(name, 4, buffer, &length, nullptr, 0) == 0 && length > 1) {
        return canonicalExecutable(std::filesystem::path(std::string(buffer)));
    }
#elif defined(__OpenBSD__)
    int name[] = {CTL_KERN, KERN_PROC_ARGS, getpid(), KERN_PROC_ARGV};
    std::size_t length = 0;
    if (sysctl(name, 4, nullptr, &length, nullptr, 0) == 0 && length > 0) {
        std::vector<char> buffer(length);
        if (sysctl(name, 4, buffer.data(), &length, nullptr, 0) == 0) {
            const auto arguments = reinterpret_cast<char* const*>(buffer.data());
            if (arguments[0] != nullptr) {
                if (auto executable = canonicalExecutable(arguments[0])) return executable;
            }
        }
    }
#endif
    return std::nullopt;
}

std::vector<std::string> splitPathList(const std::string& value) {
    std::vector<std::string> entries;
    std::size_t start = 0;
    while (start <= value.size()) {
        const auto end = value.find(':', start);
        entries.push_back(value.substr(
            start, end == std::string::npos ? std::string::npos : end - start));
        if (end == std::string::npos) break;
        start = end + 1;
    }
    return entries;
}

ResolvedPath environmentPath(const std::string& value, const std::string& variable) {
    return {normalizedAbsolute(value), variable};
}

} // namespace

std::vector<ResolvedPath> ModuleSearchPaths::packageRoots() const {
    std::vector<ResolvedPath> roots;
    std::set<std::string> seen;
    const auto add = [&roots, &seen](const ResolvedPath& root) {
        if (seen.insert(root.path.string()).second) roots.push_back(root);
    };
    add(projectModuleRoot);
    add(standardModuleRoot);
    for (const auto& root : compatibilityRoots) add(root);
    return roots;
}

InstallLayout configuredInstallLayout() {
    return {SIMP_INSTALL_BINDIR, SIMP_INSTALL_LIBDIR, SIMP_INSTALL_INCLUDEDIR,
            SIMP_INSTALL_DATADIR};
}

std::optional<std::string> processEnvironment(const std::string& name) {
    const auto* value = std::getenv(name.c_str());
    if (value == nullptr || *value == '\0') return std::nullopt;
    return std::string(value);
}

std::optional<std::filesystem::path> currentExecutablePath(const std::string& argv0) {
    if (auto platformPath = platformExecutablePath()) return platformPath;
    if (argv0.empty()) return std::nullopt;
    if (argv0.find('/') != std::string::npos) {
        return canonicalExecutable(normalizedAbsolute(argv0));
    }
    const auto searchPath = processEnvironment("PATH");
    if (!searchPath) return std::nullopt;
    for (const auto& directory : splitPathList(*searchPath)) {
        const auto base = directory.empty() ? std::filesystem::path(".")
                                            : std::filesystem::path(directory);
        if (auto found = canonicalExecutable(normalizedAbsolute(base / argv0))) {
            return found;
        }
    }
    return std::nullopt;
}

std::filesystem::path installPrefixForExecutable(const std::filesystem::path& executable,
                                                 const InstallLayout& layout) {
    auto directory = executable.parent_path().lexically_normal();
    std::vector<std::filesystem::path> binComponents;
    for (const auto& component : layout.binDirectory.lexically_normal()) {
        if (!component.empty() && component != ".") binComponents.push_back(component);
    }
    auto prefix = directory;
    bool matches = !binComponents.empty();
    for (auto component = binComponents.rbegin(); matches && component != binComponents.rend();
         ++component) {
        if (prefix.filename() != *component) {
            matches = false;
        } else {
            prefix = prefix.parent_path();
        }
    }
    return matches ? prefix : directory.parent_path();
}

ResourcePaths resolveResourcePaths(const std::optional<std::filesystem::path>& executable,
                                   const InstallLayout& layout,
                                   const EnvironmentLookup& environment) {
    ResourcePaths paths;
    paths.executable = executable;
    if (const auto home = environment("SIMP_HOME")) {
        paths.prefix = environmentPath(*home, "SIMP_HOME");
    } else if (executable) {
        paths.prefix = ResolvedPath{installPrefixForExecutable(*executable, layout),
                                    "executable-relative"};
    }
    const auto resolve = [&](const std::string& variable,
                             const std::filesystem::path& relative) -> ResolvedPath {
        if (const auto value = environment(variable)) return environmentPath(*value, variable);
        if (!paths.prefix) {
            throw std::runtime_error(
                "cannot locate the simp executable to find its resources; set " + variable +
                " or SIMP_HOME");
        }
        return {(paths.prefix->path / relative).lexically_normal(),
                paths.prefix->origin == "SIMP_HOME" ? "SIMP_HOME" : "executable-relative"};
    };
    paths.runtimeDirectory = resolve("SIMP_RUNTIME_DIR", layout.libDirectory / "simp");
    paths.runtimeLibrary = paths.runtimeDirectory.path / SIMP_RUNTIME_LIBRARY_NAME;
    paths.includeDirectory = resolve("SIMP_INCLUDE_DIR", layout.includeDirectory);
    paths.preludeDirectory =
        resolve("SIMP_PRELUDE_DIR", layout.dataDirectory / "simp" / "prelude");
    paths.preludeSource = paths.preludeDirectory.path / preludeFileName;
    paths.standardModuleDirectory =
        resolve("SIMP_STDLIB_MODULE_DIR", layout.dataDirectory / "simp" / "modules");
    return paths;
}

const ResourcePaths& processResourcePaths() {
    static const ResourcePaths paths = resolveResourcePaths(
        currentExecutablePath({}), configuredInstallLayout(), processEnvironment);
    return paths;
}

ModuleSearchPaths resolveModuleSearchPaths(const ModuleSearchRequest& request,
                                           const ResourcePaths& resources,
                                           const EnvironmentLookup& environment) {
    ModuleSearchPaths paths;
    const auto currentDirectory = normalizedAbsolute(request.currentDirectory);
    const auto absoluteFrom = [&currentDirectory](const std::filesystem::path& path) {
        return (path.is_absolute() ? path : currentDirectory / path).lexically_normal();
    };
    if (request.sourcePaths.empty()) {
        paths.projectRoot = {currentDirectory, "current directory; no source input"};
    } else {
        paths.projectRoot = {absoluteFrom(request.sourcePaths.front()).parent_path(),
                             "parent of first source input"};
    }
    if (request.moduleDirectoryOption) {
        paths.projectModuleRoot = {absoluteFrom(*request.moduleDirectoryOption),
                                   "-M/--module-dir"};
        paths.projectModuleRootExplicit = true;
    } else if (const auto moduleDirectory = environment("SIMP_MODULE_DIR")) {
        paths.projectModuleRoot = {absoluteFrom(*moduleDirectory), "SIMP_MODULE_DIR"};
        paths.projectModuleRootExplicit = true;
    } else {
        paths.projectModuleRoot = {paths.projectRoot.path / "modules",
                                   "default <project-root>/modules"};
    }
    paths.moduleSelectionFile = {paths.projectModuleRoot.path / "modules.toml",
                                 "project module selection"};
    paths.standardModuleRoot = resources.standardModuleDirectory;
    for (const auto& option : request.packagePathOptions) {
        if (option.empty()) continue;
        paths.compatibilityRoots.push_back(
            {absoluteFrom(option), "--package-path, deprecated"});
    }
    if (!request.packagePathOptions.empty()) {
        paths.deprecationWarnings.push_back(
            "--package-path is deprecated; place packages under the project module root "
            "(-M/--module-dir, SIMP_MODULE_DIR, or <project-root>/modules)");
    }
    if (const auto packagePath = environment("SIMP_PACKAGE_PATH")) {
        for (const auto& entry : splitPathList(*packagePath)) {
            if (entry.empty()) continue;
            paths.compatibilityRoots.push_back(
                {absoluteFrom(entry), "SIMP_PACKAGE_PATH, deprecated"});
        }
        paths.deprecationWarnings.push_back(
            "SIMP_PACKAGE_PATH is deprecated; use SIMP_MODULE_DIR or -M/--module-dir");
    }
    if (const auto registry = environment("SIMP_MODULE_REGISTRY")) {
        paths.registry = {absoluteFrom(*registry), "SIMP_MODULE_REGISTRY, deprecated"};
    } else {
        paths.registry = {currentDirectory / "simp-modules.tsv",
                          "default ./simp-modules.tsv, deprecated"};
    }
    return paths;
}

void validateModuleSearchPaths(const ModuleSearchPaths& paths) {
    if (!paths.projectModuleRootExplicit) return;
    std::error_code error;
    if (!std::filesystem::is_directory(paths.projectModuleRoot.path, error)) {
        throw std::runtime_error("module directory from " + paths.projectModuleRoot.origin +
                                 " does not exist or is not a directory: " +
                                 paths.projectModuleRoot.path.string());
    }
}

} // namespace simp
