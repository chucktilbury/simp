#include "test_cases.hpp"

#include "simp/PathResolution.hpp"

#include <filesystem>
#include <chrono>
#include <fstream>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using namespace simp_test;

simp::EnvironmentLookup environmentOf(std::map<std::string, std::string> values) {
    return [values](const std::string& name) -> std::optional<std::string> {
        const auto found = values.find(name);
        if (found == values.end()) return std::nullopt;
        return found->second;
    };
}

simp::InstallLayout testLayout() {
    return {"bin", "lib64", "include", "share"};
}

void requirePath(const std::filesystem::path& actual, const std::string& expected,
                 const std::string& message) {
    require(actual == std::filesystem::path(expected),
            message + ": expected " + expected + ", got " + actual.string());
}

std::filesystem::path missingDirectory() {
    return std::filesystem::temp_directory_path() /
           ("simp-path-tests-missing-" +
            std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
}

class ProjectFixture {
public:
    ProjectFixture() : root(std::filesystem::temp_directory_path() /
                            ("simp-path-project-" +
                             std::to_string(std::chrono::steady_clock::now()
                                                .time_since_epoch().count()))) {
        std::filesystem::create_directories(root / "src/nested");
        std::filesystem::create_directories(root / "modules");
        std::ofstream(root / "simpkg.toml") << "schema = 1\n";
        std::ofstream(root / "simpkg.lock") << "schema = 1\n";
        std::ofstream(root / "src/nested/main.simp") << "start {}\n";
    }
    ~ProjectFixture() {
        std::error_code error;
        std::filesystem::remove_all(root, error);
    }
    std::filesystem::path root;
};

void expectFailure(const std::function<void()>& action, const std::string& expected) {
    try {
        action();
    } catch (const std::runtime_error& error) {
        require(std::string(error.what()).find(expected) != std::string::npos,
                "expected '" + expected + "', got: " + error.what());
        return;
    }
    throw std::runtime_error("expected error: " + expected);
}

const TestGroupRegistration registration{6, {
        {"resources resolve relative to the executable prefix", [] {
             const auto paths = simp::resolveResourcePaths(
                 std::filesystem::path("/opt/simp/bin/simp"), testLayout(), environmentOf({}));
             require(paths.prefix && paths.prefix->origin == "executable-relative",
                     "the executable should determine the prefix");
             requirePath(paths.prefix->path, "/opt/simp", "prefix");
             requirePath(paths.runtimeDirectory.path, "/opt/simp/lib64/simp", "runtime dir");
             require(paths.runtimeLibrary.parent_path() == paths.runtimeDirectory.path,
                     "the runtime library should live in the runtime directory");
             requirePath(paths.includeDirectory.path, "/opt/simp/include", "include dir");
             requirePath(paths.builtinSource, "/opt/simp/share/simp/builtin/String.simp",
                         "builtin source");
             requirePath(paths.standardModuleDirectory.path, "/opt/simp/share/simp/modules",
                         "standard modules");
         }},
        {"SIMP_HOME replaces the prefix and resource variables override it", [] {
             const auto paths = simp::resolveResourcePaths(
                 std::filesystem::path("/opt/simp/bin/simp"), testLayout(),
                 environmentOf({{"SIMP_HOME", "/home/me/simp/"},
                                {"SIMP_RUNTIME_DIR", "/runtime"},
                                {"SIMP_INCLUDE_DIR", "/headers"},
                                {"SIMP_BUILTIN_DIR", "/builtin"},
                                {"SIMP_STDLIB_MODULE_DIR", "/stdlib"}}));
             require(paths.prefix && paths.prefix->origin == "SIMP_HOME",
                     "SIMP_HOME should replace the executable prefix");
             requirePath(paths.runtimeDirectory.path, "/runtime", "runtime override");
             require(paths.runtimeDirectory.origin == "SIMP_RUNTIME_DIR",
                     "runtime origin should name its variable");
             requirePath(paths.includeDirectory.path, "/headers", "include override");
             requirePath(paths.builtinSource, "/builtin/String.simp", "builtin override");
             require(paths.builtinDirectory.origin == "SIMP_BUILTIN_DIR",
                     "builtin origin should name its variable");
             requirePath(paths.standardModuleDirectory.path, "/stdlib", "stdlib override");

             const auto home = simp::resolveResourcePaths(
                 std::nullopt, testLayout(), environmentOf({{"SIMP_HOME", "/home/me/simp"}}));
             requirePath(home.runtimeDirectory.path, "/home/me/simp/lib64/simp",
                         "SIMP_HOME runtime dir");
             require(home.runtimeDirectory.origin == "SIMP_HOME",
                     "SIMP_HOME-relative resources should report SIMP_HOME");
             requirePath(home.builtinSource, "/home/me/simp/share/simp/builtin/String.simp",
                         "SIMP_HOME builtin source");
             require(home.builtinDirectory.origin == "SIMP_HOME",
                     "SIMP_HOME-relative builtins should report SIMP_HOME");
         }},
        {"an unknown executable without overrides is an explicit error", [] {
             try {
                 static_cast<void>(simp::resolveResourcePaths(std::nullopt, testLayout(),
                                                              environmentOf({})));
             } catch (const std::runtime_error& error) {
                 require(std::string(error.what()).find("SIMP_HOME") != std::string::npos,
                         "the error should name the SIMP_HOME override");
                 return;
             }
             throw std::runtime_error("expected unresolved resources to fail");
         }},
        {"install prefixes strip multi-component binary directories", [] {
             requirePath(simp::installPrefixForExecutable("/usr/local/bin/simp", testLayout()),
                         "/usr/local", "single bindir");
             simp::InstallLayout nested = testLayout();
             nested.binDirectory = "tools/bin";
             requirePath(simp::installPrefixForExecutable("/opt/x/tools/bin/simp", nested),
                         "/opt/x", "nested bindir");
             requirePath(simp::installPrefixForExecutable("/opt/x/other/simp", nested),
                         "/opt/x", "unmatched bindir falls back to the parent");
         }},
        {"package roots use CLI, project, environment, user, and installation precedence", [] {
             const auto resources = simp::resolveResourcePaths(
                 std::filesystem::path("/opt/simp/bin/simp"), testLayout(), environmentOf({}));
             ProjectFixture project;
             simp::ModuleSearchRequest request;
             request.sourcePaths = {(project.root / "src/nested/main.simp").string()};
             request.currentDirectory = project.root / "src/nested";
             request.moduleDirectoryOption = "cli-modules";
             const auto paths = simp::resolveModuleSearchPaths(
                 request, resources,
                 environmentOf({{"SIMP_MODULE_DIR", "env-modules"},
                                {"XDG_CONFIG_HOME", "/user/config"}}));
             requirePath(paths.projectRoot.path, project.root.string(), "discovered project");
             requirePath(paths.projectModuleRoot.path, (project.root / "modules").string(),
                         "project package root");
             requirePath(paths.projectLockFile.path, (project.root / "simpkg.lock").string(),
                         "manifest lock remains project-scoped");
             requirePath(paths.commandLineModuleRoot->path,
                         (project.root / "src/nested/cli-modules").string(), "CLI root");
             requirePath(paths.environmentModuleRoot->path,
                         (project.root / "src/nested/env-modules").string(), "environment root");
             requirePath(paths.userModuleRoot->path, "/user/config/simp/modules", "user root");
             const auto roots = paths.packageRoots();
             std::vector<std::filesystem::path> rootPaths;
             for (const auto& root : roots) rootPaths.push_back(root.path);
             require(rootPaths == std::vector<std::filesystem::path>{
                                      project.root / "src/nested/cli-modules",
                                      project.root / "modules",
                                      project.root / "src/nested/env-modules",
                                      "/user/config/simp/modules",
                                      "/opt/simp/share/simp/modules"},
                     "package roots must follow increasing precedence");
             require(paths.projectLockFile.path != paths.commandLineModuleRoot->path /
                                                               "simpkg.lock",
                     "CLI storage roots must not relocate project configuration");
         }},
        {"project discovery works for absolute sources and missing explicit roots fail", [] {
             const auto resources = simp::resolveResourcePaths(
                 std::filesystem::path("/opt/simp/bin/simp"), testLayout(), environmentOf({}));
             ProjectFixture project;
             simp::ModuleSearchRequest request;
             request.sourcePaths = {(project.root / "src/nested/main.simp").string()};
             request.currentDirectory = "/unrelated";
             const auto paths = simp::resolveModuleSearchPaths(request, resources, environmentOf({}));
             requirePath(paths.projectRoot.path, project.root.string(),
                         "absolute source ancestor discovery");
             request.moduleDirectoryOption = missingDirectory().string();
             expectFailure(
                 [&] {
                     simp::validateModuleSearchPaths(simp::resolveModuleSearchPaths(
                         request, resources, environmentOf({{"SIMP_MODULE_DIR", "/missing/env"}})));
                 },
                 "-M/--module-dir");
             request.moduleDirectoryOption.reset();
             expectFailure(
                 [&] {
                     simp::validateModuleSearchPaths(simp::resolveModuleSearchPaths(
                         request, resources, environmentOf({{"SIMP_MODULE_DIR", "/missing/env"}})));
                 },
                 "SIMP_MODULE_DIR");
         }},
        {"legacy package lookup and non-absolute XDG paths are rejected", [] {
             const auto resources = simp::resolveResourcePaths(
                 std::filesystem::path("/opt/simp/bin/simp"), testLayout(), environmentOf({}));
             simp::ModuleSearchRequest request;
             request.currentDirectory = "/work";
             expectFailure(
                 [&] {
                     simp::resolveModuleSearchPaths(
                         request, resources, environmentOf({{"SIMP_PACKAGE_PATH", "/old"}}));
                 },
                 "SIMP_PACKAGE_PATH is no longer supported");
             expectFailure(
                 [&] {
                     simp::resolveModuleSearchPaths(
                         request, resources,
                         environmentOf({{"SIMP_MODULE_REGISTRY", "/old/registry.tsv"}}));
                 },
                 "SIMP_MODULE_REGISTRY is no longer supported");
             expectFailure(
                 [&] {
                     simp::resolveModuleSearchPaths(
                         request, resources, environmentOf({{"XDG_CONFIG_HOME", "relative"}}));
                 },
                 "XDG_CONFIG_HOME must be an absolute path");
         }},
        {"only explicitly selected missing module roots are errors", [] {
             const auto resources = simp::resolveResourcePaths(
                 std::filesystem::path("/opt/simp/bin/simp"), testLayout(), environmentOf({}));
             simp::ModuleSearchRequest request;
             request.currentDirectory = missingDirectory();
             simp::validateModuleSearchPaths(
                 simp::resolveModuleSearchPaths(request, resources, environmentOf({})));
             request.moduleDirectoryOption = missingDirectory().string();
             try {
                 simp::validateModuleSearchPaths(
                     simp::resolveModuleSearchPaths(request, resources, environmentOf({})));
             } catch (const std::runtime_error& error) {
                 require(std::string(error.what()).find("-M/--module-dir") !=
                             std::string::npos,
                         "the error should name the selecting option");
                 return;
             }
             throw std::runtime_error("expected a missing -M root to fail");
         }},
}};

} // namespace
