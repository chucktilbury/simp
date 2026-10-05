#include "test_cases.hpp"

#include "simp/PathResolution.hpp"

#include <filesystem>
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
    return std::filesystem::temp_directory_path() / "simp-path-tests-missing-directory";
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
        {"module roots prefer -M, then SIMP_MODULE_DIR, then the source parent", [] {
             const auto resources = simp::resolveResourcePaths(
                 std::filesystem::path("/opt/simp/bin/simp"), testLayout(), environmentOf({}));
             simp::ModuleSearchRequest request;
             request.sourcePaths = {"app/main.simp", "lib/other.simp"};
             request.currentDirectory = "/work";
             auto paths = simp::resolveModuleSearchPaths(request, resources, environmentOf({}));
             requirePath(paths.projectRoot.path, "/work/app", "project root");
             requirePath(paths.projectModuleRoot.path, "/work/app/modules", "default root");
             requirePath(paths.moduleSelectionFile.path, "/work/app/modules/modules.toml",
                         "default module selection file");
             require(!paths.projectModuleRootExplicit && paths.deprecationWarnings.empty(),
                     "the default root should be implicit and warning-free");
             requirePath(paths.registry.path, "/work/simp-modules.tsv", "default registry");

             const auto environment = environmentOf({{"SIMP_MODULE_DIR", "/env/modules"}});
             paths = simp::resolveModuleSearchPaths(request, resources, environment);
             requirePath(paths.projectModuleRoot.path, "/env/modules", "environment root");
             requirePath(paths.moduleSelectionFile.path, "/env/modules/modules.toml",
                         "environment module selection file");
             require(paths.projectModuleRoot.origin == "SIMP_MODULE_DIR",
                     "environment root origin");

             request.moduleDirectoryOption = "deps";
             paths = simp::resolveModuleSearchPaths(request, resources, environment);
             requirePath(paths.projectModuleRoot.path, "/work/deps", "command-line root");
             requirePath(paths.moduleSelectionFile.path, "/work/deps/modules.toml",
                         "command-line module selection file");
             require(paths.projectModuleRoot.origin == "-M/--module-dir",
                     "command-line root origin");

             request = {};
             request.currentDirectory = "/work";
             paths = simp::resolveModuleSearchPaths(request, resources, environmentOf({}));
             requirePath(paths.projectModuleRoot.path, "/work/modules",
                         "without sources the current directory is the project root");
         }},
        {"compatibility roots follow canonical roots and warn", [] {
             const auto resources = simp::resolveResourcePaths(
                 std::filesystem::path("/opt/simp/bin/simp"), testLayout(), environmentOf({}));
             simp::ModuleSearchRequest request;
             request.sourcePaths = {"/src/main.simp"};
             request.currentDirectory = "/work";
             request.packagePathOptions = {"old", "/src/modules"};
             const auto paths = simp::resolveModuleSearchPaths(
                 request, resources,
                 environmentOf({{"SIMP_PACKAGE_PATH", "/a::/b"},
                                {"SIMP_MODULE_REGISTRY", "registry.tsv"}}));
             require(paths.deprecationWarnings.size() == 2,
                     "both deprecated sources should warn");
             const auto roots = paths.packageRoots();
             std::vector<std::filesystem::path> rootPaths;
             for (const auto& root : roots) rootPaths.push_back(root.path);
             require(rootPaths == std::vector<std::filesystem::path>{
                                      "/src/modules", "/opt/simp/share/simp/modules",
                                      "/work/old", "/a", "/b"},
                     "roots should be ordered project, standard, compatibility and deduped");
             requirePath(paths.registry.path, "/work/registry.tsv", "registry override");
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
