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

constexpr const char* builtinFileName = "String.simp";

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
    if (commandLineModuleRoot) add(*commandLineModuleRoot);
    add(projectModuleRoot);
    if (environmentModuleRoot) add(*environmentModuleRoot);
    if (userModuleRoot) add(*userModuleRoot);
    add(standardModuleRoot);
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
    paths.builtinDirectory =
        resolve("SIMP_BUILTIN_DIR", layout.dataDirectory / "simp" / "builtin");
    paths.builtinSource = paths.builtinDirectory.path / builtinFileName;
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
    for (auto candidate = paths.projectRoot.path; !candidate.empty();) {
        if (std::filesystem::exists(candidate / "simpkg.toml") ||
            std::filesystem::is_directory(candidate / "modules")) {
            if (candidate != paths.projectRoot.path) {
                paths.projectRoot = {candidate, "nearest source ancestor with simpkg.toml or modules"};
            }
            break;
        }
        const auto parent = candidate.parent_path();
        if (parent == candidate) break;
        candidate = parent;
    }
    if (const auto legacyPackagePath = environment("SIMP_PACKAGE_PATH")) {
        static_cast<void>(legacyPackagePath);
        throw std::runtime_error(
            "SIMP_PACKAGE_PATH is no longer supported; use SIMP_MODULE_DIR or -M/--module-dir");
    }
    if (environment("SIMP_MODULE_REGISTRY")) {
        throw std::runtime_error(
            "SIMP_MODULE_REGISTRY is no longer supported; publish packages with "
            "simp-package.toml and install them with simpkg");
    }
    paths.projectModuleRoot = {paths.projectRoot.path / "modules",
                               "project <project-root>/modules"};
    if (request.moduleDirectoryOption) {
        paths.commandLineModuleRoot = {
            absoluteFrom(*request.moduleDirectoryOption), "-M/--module-dir"};
    }
    if (const auto moduleDirectory = environment("SIMP_MODULE_DIR")) {
        paths.environmentModuleRoot =
            ResolvedPath{absoluteFrom(*moduleDirectory), "SIMP_MODULE_DIR"};
    }
    std::optional<std::filesystem::path> configDirectory;
    std::string userRootOrigin;
    if (const auto xdgConfigHome = environment("XDG_CONFIG_HOME")) {
        const std::filesystem::path configured(*xdgConfigHome);
        if (!configured.is_absolute()) {
            throw std::runtime_error("XDG_CONFIG_HOME must be an absolute path");
        }
        configDirectory = configured;
        userRootOrigin = "XDG_CONFIG_HOME/simp/modules";
    } else if (const auto home = environment("HOME")) {
        configDirectory = std::filesystem::path(*home) / ".config";
        userRootOrigin = "HOME/.config/simp/modules";
    }
    if (configDirectory) {
        paths.userModuleRoot = ResolvedPath{*configDirectory / "simp" / "modules",
                                            userRootOrigin};
    }
    paths.standardModuleRoot = resources.standardModuleDirectory;
    paths.projectLockFile = {paths.projectRoot.path / "simpkg.lock", "project lock"};
    const auto manifest = paths.projectRoot.path / "simpkg.toml";
    const auto legacyPolicy = paths.projectRoot.path / "modules" / "modules.toml";
    if (std::filesystem::exists(legacyPolicy)) {
        throw std::runtime_error(
            "legacy modules/modules.toml is no longer supported; migrate the project "
            "to simpkg.toml and simpkg.lock with 'simpkg init'");
    }
    if (std::filesystem::exists(paths.projectRoot.path / "simpkg.lock") &&
        !std::filesystem::exists(manifest)) {
        throw std::runtime_error("simpkg.lock exists without simpkg.toml in project " +
                                 paths.projectRoot.path.string());
    }
    if (std::filesystem::exists(manifest) &&
        !std::filesystem::is_regular_file(paths.projectLockFile.path)) {
        throw std::runtime_error("missing simpkg.lock; run 'simpkg install --yes'");
    }
    for (const auto& legacyRegistry : {currentDirectory / "simp-modules.tsv",
                                       paths.projectRoot.path / "simp-modules.tsv"}) {
        if (std::filesystem::exists(legacyRegistry)) {
            throw std::runtime_error(
                "simp-modules.tsv registries are no longer supported; publish packages "
                "with simp-package.toml and install them with simpkg");
        }
    }
    return paths;
}

void validateModuleSearchPaths(const ModuleSearchPaths& paths) {
    const auto validate = [](const std::optional<ResolvedPath>& root) {
        if (!root) return;
        std::error_code error;
        if (!std::filesystem::is_directory(root->path, error)) {
            throw std::runtime_error("module directory from " + root->origin +
                                     " does not exist or is not a directory: " +
                                     root->path.string());
        }
    };
    validate(paths.commandLineModuleRoot);
    validate(paths.environmentModuleRoot);
}

} // namespace simp
