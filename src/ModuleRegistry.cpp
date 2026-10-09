#include "cwhip/ModuleRegistry.hpp"

#include "cwhip/Diagnostic.hpp"
#include "cwhip/Lexer.hpp"
#include "cwhip/PackageRegistry.hpp"
#include "cwhip/Parser.hpp"
#include "cwhip/SourceLoader.hpp"

#include <algorithm>
#include <fstream>
#include <iterator>
#include <set>
#include <unordered_map>

namespace cwhip {
namespace {

struct RegistryEntry {
    std::string name;
    std::filesystem::path sourcePath;
    std::string version;
    std::vector<std::string> dependencyVersions;
    std::string exportKind;
    std::string exportName;
};

using Registry = std::unordered_map<std::string, RegistryEntry>;

std::string describeSearchedPath(const ResolvedPath& path, bool directory) {
    std::error_code error;
    const bool present = directory ? std::filesystem::is_directory(path.path, error)
                                   : std::filesystem::is_regular_file(path.path, error);
    return "  " + path.path.string() + (present ? "" : " [not found]") + " (" +
           path.origin + ")";
}

std::string missingModuleMessage(const std::string& moduleName,
                                 const ModuleLoadOptions& options) {
    std::string message = "module '" + moduleName +
                           "' was not found in any package module root; expected <module-root>/" +
                           moduleName + "/<version>/cwhip-package.toml\nsearched module roots:";
    for (const auto& root : options.packageSearchRoots) {
        message += "\n" + describeSearchedPath(root, true);
    }
    return message;
}

struct ModuleSource {
    LoadedModule info;
    Program program;
    enum class State { Loading, Loaded } state = State::Loading;
};

} // namespace

ModuleLoadResult loadImportedModules(Program& program,
                                     const ModuleLoadOptions& options) {
    const auto policy = readModuleVersionPolicy(options.projectLockFile);
    ModuleLoadResult result;
    result.versionPolicyActive = policy.has_value();
    if (program.imports.empty()) return result;
    Registry registry;
    std::vector<std::string> rootNames;
    rootNames.reserve(program.imports.size());
    for (const auto& import : program.imports) rootNames.push_back(import.moduleName);
    std::vector<std::filesystem::path> searchRoots;
    searchRoots.reserve(options.packageSearchRoots.size());
    for (const auto& root : options.packageSearchRoots) searchRoots.push_back(root.path);
    auto packages = resolvePackages(rootNames, searchRoots, policy);
    const auto addPackageEntries = [&registry](const PackageResolution& resolution) {
        for (const auto& [name, package] : resolution.packages) {
            RegistryEntry entry;
            entry.name = name;
            entry.sourcePath = package.sourcePath;
            entry.version = package.version;
            entry.exportKind = package.exportKind;
            entry.exportName = package.exportName;
            for (const auto& dependency : package.dependencies) {
                entry.dependencyVersions.push_back(dependency.name + "=" +
                                                   dependency.version);
            }
            registry[name] = std::move(entry);
        }
    };
    addPackageEntries(packages);
    std::unordered_map<std::string, ModuleSource> modules;
    std::vector<std::string> order;

    const auto load = [&](const auto& self, ImportDeclaration& import,
                          const std::string& importerModule) -> void {
        import.importerModule = importerModule;
        const auto ownerPackage = packages.packages.find(importerModule);
        if (ownerPackage != packages.packages.end()) {
            const bool declaredDependency =
                std::any_of(ownerPackage->second.dependencies.begin(),
                            ownerPackage->second.dependencies.end(),
                            [&import](const PackageDependency& dependency) {
                                return dependency.name == import.moduleName;
                            });
            if (!declaredDependency) {
                throw DiagnosticError(
                    import.location, "package '" + importerModule +
                                         "' imports package '" + import.moduleName +
                                         "' without declaring it in [dependencies]");
            }
        }
        if (packages.packages.find(import.moduleName) == packages.packages.end()) {
            const auto additional = resolvePackages({import.moduleName}, searchRoots, policy);
            for (const auto& [name, package] : additional.packages) {
                const auto existing = packages.packages.find(name);
                if (existing != packages.packages.end() &&
                    existing->second.version != package.version) {
                    throw DiagnosticError(
                        import.location, "conflicting package versions for '" + name +
                                             "': " + existing->second.version + " and " +
                                             package.version);
                }
                packages.packages.emplace(name, package);
            }
            for (const auto& package : additional.linkOrder) {
                const auto existing = std::find_if(
                    packages.linkOrder.begin(), packages.linkOrder.end(),
                    [&package](const ResolvedPackage& item) {
                        return item.name == package.name;
                    });
                if (existing == packages.linkOrder.end()) {
                    packages.linkOrder.push_back(package);
                }
            }
            addPackageEntries(additional);
        }
        const auto entry = registry.find(import.moduleName);
        if (entry == registry.end()) {
            throw DiagnosticError(import.location,
                                  missingModuleMessage(import.moduleName, options));
        }
        const auto entryData = entry->second;
        if (import.alias.empty()) import.alias = entryData.exportName;
        auto found = modules.find(import.moduleName);
        if (found != modules.end()) {
            if (found->second.state == ModuleSource::State::Loading) {
                throw DiagnosticError(import.location,
                                      "cyclic module import involving '" +
                                          import.moduleName + "'");
            }
            import.exportedName = entryData.exportName;
            import.exportsNamespace = entryData.exportKind == "namespace";
            return;
        }
        {
            std::error_code error;
            const auto canonical = std::filesystem::canonical(entryData.sourcePath, error);
            if (error) {
                throw DiagnosticError(import.location,
                                      "cannot resolve source for module '" + import.moduleName +
                                          "': " + entryData.sourcePath.string());
            }
            std::ifstream sourceFile(canonical);
            if (!sourceFile) {
                throw DiagnosticError(import.location,
                                      "cannot open source for module '" + import.moduleName +
                                          "': " + canonical.string());
            }
            const std::string source{std::istreambuf_iterator<char>(sourceFile),
                                     std::istreambuf_iterator<char>()};
            std::unordered_set<std::string> included{canonical.string()};
            auto tokens = tokenizeWithIncludes(source, canonical, included, 0, true);
            tokens.push_back({TokenType::End, "", {canonical.string(), 1, 1}});
            ModuleSource module;
            module.info = {import.moduleName, entryData.version,
                           entryData.dependencyVersions, canonical};
            module.program = Parser(std::move(tokens)).parseModule();
            const bool exportedClass = std::any_of(
                module.program.classes.begin(), module.program.classes.end(),
                [&entryData](const ClassDeclaration& declaration) {
                    return declaration.namespacePath.empty() &&
                           declaration.name == entryData.exportName;
                });
            const bool exportedNamespace = std::any_of(
                module.program.namespaces.begin(), module.program.namespaces.end(),
                [&entryData](const NamespaceDeclaration& declaration) {
                    return declaration.path.size() == 1 &&
                           declaration.path.front() == entryData.exportName;
                });
            if ((entryData.exportKind == "class" && !exportedClass) ||
                (entryData.exportKind == "namespace" && !exportedNamespace)) {
                throw DiagnosticError(import.location,
                                      "module '" + import.moduleName +
                                          "' does not declare its registered top-level " +
                                          entryData.exportKind + " '" +
                                          entryData.exportName + "'");
            }
            for (auto& declaration : module.program.classes) {
                declaration.moduleName = import.moduleName;
            }
            for (auto& declaration : module.program.namespaces) {
                declaration.moduleName = import.moduleName;
            }
            for (auto& definition : module.program.outOfLineMethods) {
                definition.moduleName = import.moduleName;
            }
            modules.emplace(import.moduleName, std::move(module));
            for (auto& nestedImport : modules.at(import.moduleName).program.imports) {
                self(self, nestedImport, import.moduleName);
            }
            modules.at(import.moduleName).state = ModuleSource::State::Loaded;
            order.push_back(import.moduleName);
        }
        import.exportedName = entryData.exportName;
        import.exportsNamespace = entryData.exportKind == "namespace";
    };

    for (auto& import : program.imports) load(load, import, {});

    auto& loaded = result.modules;
    loaded.reserve(order.size());
    for (const auto& name : order) {
        auto& module = modules.at(name);
        loaded.push_back(module.info);
        for (const auto& import : module.program.imports) {
            program.imports.push_back(import);
        }
        for (auto& declaration : module.program.namespaces) {
            program.namespaces.push_back(std::move(declaration));
        }
        for (auto& declaration : module.program.classes) {
            program.classes.push_back(std::move(declaration));
        }
        for (auto& definition : module.program.outOfLineMethods) {
            program.outOfLineMethods.push_back(std::move(definition));
        }
    }
    std::set<std::string> seenPaths;
    std::set<std::string> seenLibraries;
    std::map<std::string, std::set<std::string>> librarySearchLocations;
    for (const auto& package : packages.linkOrder) {
        std::set<std::string> packagePaths;
        for (const auto& libraryPath : package.libraryPaths) {
            const auto normalized = std::filesystem::absolute(libraryPath).lexically_normal();
            packagePaths.insert(normalized.string());
            if (seenPaths.insert(normalized.string()).second) {
                result.libraryPaths.push_back(normalized);
            }
        }
        for (const auto& library : package.libraries) {
            const auto owner = librarySearchLocations.find(library);
            if (owner != librarySearchLocations.end() &&
                owner->second != packagePaths) {
                throw std::runtime_error(
                    "conflicting package library search paths for native library '" +
                    library + "'");
            }
            librarySearchLocations.emplace(library, packagePaths);
            if (seenLibraries.insert(library).second) {
                result.libraries.push_back(library);
            }
        }
    }
    return result;
}

} // namespace cwhip
