#include "test_cases.hpp"

#include "cwhip/PathResolution.hpp"

#include <filesystem>
#include <chrono>
#include <fstream>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using namespace cwhip_test;

cwhip::EnvironmentLookup environmentOf(std::map<std::string, std::string> values) {
    return [values](const std::string& name) -> std::optional<std::string> {
        const auto found = values.find(name);
        if (found == values.end()) return std::nullopt;
        return found->second;
    };
}

cwhip::InstallLayout testLayout() {
    return {"bin", "lib64", "include", "share"};
}

void requirePath(const std::filesystem::path& actual, const std::string& expected,
                 const std::string& message) {
    require(actual == std::filesystem::path(expected),
            message + ": expected " + expected + ", got " + actual.string());
}

std::filesystem::path missingDirectory() {
    return std::filesystem::temp_directory_path() /
           ("cwhip-path-tests-missing-" +
            std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
}

class ProjectFixture {
public:
    ProjectFixture() : root(std::filesystem::temp_directory_path() /
                            ("cwhip-path-project-" +
                             std::to_string(std::chrono::steady_clock::now()
                                                .time_since_epoch().count()))) {
        std::filesystem::create_directories(root / "src/nested");
        std::filesystem::create_directories(root / "modules");
        std::ofstream(root / "cwhip-pkg.toml") << "schema = 1\n";
        std::ofstream(root / "cwhip-pkg.lock") << "schema = 1\n";
        std::ofstream(root / "src/nested/main.cw") << "start {}\n";
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
             const auto paths = cwhip::resolveResourcePaths(
                 std::filesystem::path("/opt/cwhip/bin/cwhip"), testLayout(), environmentOf({}));
             require(paths.prefix && paths.prefix->origin == "executable-relative",
                     "the executable should determine the prefix");
             requirePath(paths.prefix->path, "/opt/cwhip", "prefix");
             requirePath(paths.runtimeDirectory.path, "/opt/cwhip/lib64/cwhip", "runtime dir");
             require(paths.runtimeLibrary.parent_path() == paths.runtimeDirectory.path,
                     "the runtime library should live in the runtime directory");
             requirePath(paths.includeDirectory.path, "/opt/cwhip/include", "include dir");
             requirePath(paths.builtinSource, "/opt/cwhip/share/cwhip/builtin/String.cw",
                         "builtin source");
             requirePath(paths.standardModuleDirectory.path, "/opt/cwhip/share/cwhip/modules",
                         "standard modules");
         }},
        {"CWHIP_HOME replaces the prefix and resource variables override it", [] {
             const auto paths = cwhip::resolveResourcePaths(
                 std::filesystem::path("/opt/cwhip/bin/cwhip"), testLayout(),
                 environmentOf({{"CWHIP_HOME", "/home/me/cwhip/"},
                                {"CWHIP_RUNTIME_DIR", "/runtime"},
                                {"CWHIP_INCLUDE_DIR", "/headers"},
                                {"CWHIP_BUILTIN_DIR", "/builtin"},
                                {"CWHIP_STDLIB_MODULE_DIR", "/stdlib"}}));
             require(paths.prefix && paths.prefix->origin == "CWHIP_HOME",
                     "CWHIP_HOME should replace the executable prefix");
             requirePath(paths.runtimeDirectory.path, "/runtime", "runtime override");
             require(paths.runtimeDirectory.origin == "CWHIP_RUNTIME_DIR",
                     "runtime origin should name its variable");
             requirePath(paths.includeDirectory.path, "/headers", "include override");
             requirePath(paths.builtinSource, "/builtin/String.cw", "builtin override");
             require(paths.builtinDirectory.origin == "CWHIP_BUILTIN_DIR",
                     "builtin origin should name its variable");
             requirePath(paths.standardModuleDirectory.path, "/stdlib", "stdlib override");

             const auto home = cwhip::resolveResourcePaths(
                 std::nullopt, testLayout(), environmentOf({{"CWHIP_HOME", "/home/me/cwhip"}}));
             requirePath(home.runtimeDirectory.path, "/home/me/cwhip/lib64/cwhip",
                         "CWHIP_HOME runtime dir");
             require(home.runtimeDirectory.origin == "CWHIP_HOME",
                     "CWHIP_HOME-relative resources should report CWHIP_HOME");
             requirePath(home.builtinSource, "/home/me/cwhip/share/cwhip/builtin/String.cw",
                         "CWHIP_HOME builtin source");
             require(home.builtinDirectory.origin == "CWHIP_HOME",
                     "CWHIP_HOME-relative builtins should report CWHIP_HOME");
         }},
        {"an unknown executable without overrides is an explicit error", [] {
             try {
                 static_cast<void>(cwhip::resolveResourcePaths(std::nullopt, testLayout(),
                                                              environmentOf({})));
             } catch (const std::runtime_error& error) {
                 require(std::string(error.what()).find("CWHIP_HOME") != std::string::npos,
                         "the error should name the CWHIP_HOME override");
                 return;
             }
             throw std::runtime_error("expected unresolved resources to fail");
         }},
        {"install prefixes strip multi-component binary directories", [] {
             requirePath(cwhip::installPrefixForExecutable("/usr/local/bin/cwhip", testLayout()),
                         "/usr/local", "single bindir");
             cwhip::InstallLayout nested = testLayout();
             nested.binDirectory = "tools/bin";
             requirePath(cwhip::installPrefixForExecutable("/opt/x/tools/bin/cwhip", nested),
                         "/opt/x", "nested bindir");
             requirePath(cwhip::installPrefixForExecutable("/opt/x/other/cwhip", nested),
                         "/opt/x", "unmatched bindir falls back to the parent");
         }},
        {"package roots use CLI, project, environment, user, and installation precedence", [] {
             const auto resources = cwhip::resolveResourcePaths(
                 std::filesystem::path("/opt/cwhip/bin/cwhip"), testLayout(), environmentOf({}));
             ProjectFixture project;
             cwhip::ModuleSearchRequest request;
             request.sourcePaths = {(project.root / "src/nested/main.cw").string()};
             request.currentDirectory = project.root / "src/nested";
             request.moduleDirectoryOption = "cli-modules";
             const auto paths = cwhip::resolveModuleSearchPaths(
                 request, resources,
                 environmentOf({{"CWHIP_MODULE_DIR", "env-modules"},
                                {"XDG_CONFIG_HOME", "/user/config"}}));
             requirePath(paths.projectRoot.path, project.root.string(), "discovered project");
             requirePath(paths.projectModuleRoot.path, (project.root / "modules").string(),
                         "project package root");
             requirePath(paths.projectLockFile.path, (project.root / "cwhip-pkg.lock").string(),
                         "manifest lock remains project-scoped");
             requirePath(paths.commandLineModuleRoot->path,
                         (project.root / "src/nested/cli-modules").string(), "CLI root");
             requirePath(paths.environmentModuleRoot->path,
                         (project.root / "src/nested/env-modules").string(), "environment root");
             requirePath(paths.userModuleRoot->path, "/user/config/cwhip/modules", "user root");
             const auto roots = paths.packageRoots();
             std::vector<std::filesystem::path> rootPaths;
             for (const auto& root : roots) rootPaths.push_back(root.path);
             require(rootPaths == std::vector<std::filesystem::path>{
                                      project.root / "src/nested/cli-modules",
                                      project.root / "modules",
                                      project.root / "src/nested/env-modules",
                                      "/user/config/cwhip/modules",
                                      "/opt/cwhip/share/cwhip/modules"},
                     "package roots must follow increasing precedence");
             require(paths.projectLockFile.path != paths.commandLineModuleRoot->path /
                                                               "cwhip-pkg.lock",
                     "CLI storage roots must not relocate project configuration");
         }},
        {"project discovery works for absolute sources and missing explicit roots fail", [] {
             const auto resources = cwhip::resolveResourcePaths(
                 std::filesystem::path("/opt/cwhip/bin/cwhip"), testLayout(), environmentOf({}));
             ProjectFixture project;
             cwhip::ModuleSearchRequest request;
             request.sourcePaths = {(project.root / "src/nested/main.cw").string()};
             request.currentDirectory = "/unrelated";
             const auto paths = cwhip::resolveModuleSearchPaths(request, resources, environmentOf({}));
             requirePath(paths.projectRoot.path, project.root.string(),
                         "absolute source ancestor discovery");
             request.moduleDirectoryOption = missingDirectory().string();
             expectFailure(
                 [&] {
                     cwhip::validateModuleSearchPaths(cwhip::resolveModuleSearchPaths(
                         request, resources, environmentOf({{"CWHIP_MODULE_DIR", "/missing/env"}})));
                 },
                 "-M/--module-dir");
             request.moduleDirectoryOption.reset();
             expectFailure(
                 [&] {
                     cwhip::validateModuleSearchPaths(cwhip::resolveModuleSearchPaths(
                         request, resources, environmentOf({{"CWHIP_MODULE_DIR", "/missing/env"}})));
                 },
                 "CWHIP_MODULE_DIR");
         }},
        {"legacy package lookup and non-absolute XDG paths are rejected", [] {
             const auto resources = cwhip::resolveResourcePaths(
                 std::filesystem::path("/opt/cwhip/bin/cwhip"), testLayout(), environmentOf({}));
             cwhip::ModuleSearchRequest request;
             request.currentDirectory = "/work";
             expectFailure(
                 [&] {
                     cwhip::resolveModuleSearchPaths(
                         request, resources, environmentOf({{"CWHIP_PACKAGE_PATH", "/old"}}));
                 },
                 "CWHIP_PACKAGE_PATH is no longer supported");
             expectFailure(
                 [&] {
                     cwhip::resolveModuleSearchPaths(
                         request, resources,
                         environmentOf({{"CWHIP_MODULE_REGISTRY", "/old/registry.tsv"}}));
                 },
                 "CWHIP_MODULE_REGISTRY is no longer supported");
             expectFailure(
                 [&] {
                     cwhip::resolveModuleSearchPaths(
                         request, resources, environmentOf({{"XDG_CONFIG_HOME", "relative"}}));
                 },
                 "XDG_CONFIG_HOME must be an absolute path");
         }},
        {"only explicitly selected missing module roots are errors", [] {
             const auto resources = cwhip::resolveResourcePaths(
                 std::filesystem::path("/opt/cwhip/bin/cwhip"), testLayout(), environmentOf({}));
             cwhip::ModuleSearchRequest request;
             request.currentDirectory = missingDirectory();
             cwhip::validateModuleSearchPaths(
                 cwhip::resolveModuleSearchPaths(request, resources, environmentOf({})));
             request.moduleDirectoryOption = missingDirectory().string();
             try {
                 cwhip::validateModuleSearchPaths(
                     cwhip::resolveModuleSearchPaths(request, resources, environmentOf({})));
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
