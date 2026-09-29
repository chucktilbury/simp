#include "simp/ModuleRegistry.hpp"

#include "simp/Diagnostic.hpp"
#include "simp/Lexer.hpp"
#include "simp/Parser.hpp"
#include "simp/SourceLoader.hpp"

#include <algorithm>
#include <fstream>
#include <iterator>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

namespace simp {
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

bool identifier(const std::string& value) {
    const auto isStart = [](char character) {
        return (character >= 'a' && character <= 'z') ||
               (character >= 'A' && character <= 'Z') || character == '_';
    };
    const auto isPart = [&isStart](char character) {
        return isStart(character) || (character >= '0' && character <= '9');
    };
    if (value.empty() || !isStart(value.front())) {
        return false;
    }
    for (const char character : value) {
        if (!isPart(character)) return false;
    }
    return true;
}

std::vector<std::string> split(const std::string& value, char delimiter) {
    std::vector<std::string> parts;
    std::stringstream stream(value);
    std::string part;
    while (std::getline(stream, part, delimiter)) parts.push_back(part);
    if (!value.empty() && value.back() == delimiter) parts.emplace_back();
    return parts;
}

Registry readRegistry(const std::filesystem::path& path) {
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("cannot open module registry: " + path.string());
    }
    Registry registry;
    std::string line;
    std::size_t lineNumber = 0;
    while (std::getline(input, line)) {
        ++lineNumber;
        if (line.empty() || line.front() == '#') continue;
        const auto columns = split(line, '\t');
        if (columns.size() != 6 || !identifier(columns[0]) || columns[1].empty() ||
            columns[2].empty() || (columns[4] != "class" && columns[4] != "namespace") ||
            !identifier(columns[5])) {
            throw std::runtime_error(path.string() + ":" + std::to_string(lineNumber) +
                                     ": expected six tab-separated fields: "
                                     "module, source, version, dependencies, export-kind, export-name");
        }
        RegistryEntry entry;
        entry.name = columns[0];
        entry.sourcePath = columns[1];
        if (entry.sourcePath.is_relative()) entry.sourcePath = path.parent_path() / entry.sourcePath;
        entry.version = columns[2];
        entry.exportKind = columns[4];
        entry.exportName = columns[5];
        if (!columns[3].empty()) {
            for (const auto& dependency : split(columns[3], ',')) {
                const auto separator = dependency.find('=');
                if (separator == std::string::npos || separator == 0 ||
                    separator + 1 == dependency.size()) {
                    throw std::runtime_error(
                        path.string() + ":" + std::to_string(lineNumber) +
                        ": dependencies must be comma-separated name=version pairs");
                }
                entry.dependencyVersions.push_back(dependency);
            }
        }
        if (!registry.emplace(entry.name, std::move(entry)).second) {
            throw std::runtime_error(path.string() + ":" + std::to_string(lineNumber) +
                                     ": duplicate module name");
        }
    }
    return registry;
}

struct ModuleSource {
    LoadedModule info;
    Program program;
    enum class State { Loading, Loaded } state = State::Loading;
};

} // namespace

std::vector<LoadedModule> loadImportedModules(
    Program& program, const std::filesystem::path& registryPath) {
    if (program.imports.empty()) return {};
    const auto registry = readRegistry(registryPath);
    std::unordered_map<std::string, ModuleSource> modules;
    std::vector<std::string> order;

    const auto load = [&](const auto& self, ImportDeclaration& import,
                          const std::string& importerModule) -> void {
        import.importerModule = importerModule;
        const auto entry = registry.find(import.moduleName);
        if (entry == registry.end()) {
            throw DiagnosticError(import.location,
                                  "module '" + import.moduleName +
                                      "' is not registered in '" + registryPath.string() + "'");
        }
        auto found = modules.find(import.moduleName);
        if (found != modules.end()) {
            if (found->second.state == ModuleSource::State::Loading) {
                throw DiagnosticError(import.location,
                                      "cyclic module import involving '" +
                                          import.moduleName + "'");
            }
            import.exportedName = entry->second.exportName;
            import.exportsNamespace = entry->second.exportKind == "namespace";
            return;
        }
        {
            std::error_code error;
            const auto canonical = std::filesystem::canonical(entry->second.sourcePath, error);
            if (error) {
                throw DiagnosticError(import.location,
                                      "cannot resolve source for module '" + import.moduleName +
                                          "': " + entry->second.sourcePath.string());
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
            module.info = {import.moduleName, entry->second.version,
                           entry->second.dependencyVersions, canonical};
            module.program = Parser(std::move(tokens)).parseModule();
            const bool exportedClass = std::any_of(
                module.program.classes.begin(), module.program.classes.end(),
                [&entry](const ClassDeclaration& declaration) {
                    return declaration.namespacePath.empty() &&
                           declaration.name == entry->second.exportName;
                });
            const bool exportedNamespace = std::any_of(
                module.program.namespaces.begin(), module.program.namespaces.end(),
                [&entry](const NamespaceDeclaration& declaration) {
                    return declaration.path.size() == 1 &&
                           declaration.path.front() == entry->second.exportName;
                });
            if ((entry->second.exportKind == "class" && !exportedClass) ||
                (entry->second.exportKind == "namespace" && !exportedNamespace)) {
                throw DiagnosticError(import.location,
                                      "module '" + import.moduleName +
                                          "' does not declare its registered top-level " +
                                          entry->second.exportKind + " '" +
                                          entry->second.exportName + "'");
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
        import.exportedName = entry->second.exportName;
        import.exportsNamespace = entry->second.exportKind == "namespace";
    };

    for (auto& import : program.imports) load(load, import, {});

    std::vector<LoadedModule> loaded;
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
    return loaded;
}

} // namespace simp
