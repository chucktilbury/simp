#include "test_cases.hpp"

#include "simp/PackageRegistry.hpp"

#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>

namespace {
using namespace simp_test;

class TemporaryDirectory {
public:
    TemporaryDirectory() {
        static std::atomic<unsigned> sequence{ 0 };
        path_ = std::filesystem::temp_directory_path() /
                ("simp-module-selection-" +
                 std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + "-" +
                 std::to_string(sequence++));
        std::filesystem::create_directories(path_);
    }

    ~TemporaryDirectory() {
        std::error_code ignored;
        std::filesystem::remove_all(path_, ignored);
    }

    const std::filesystem::path& path() const {
        return path_;
    }

private:
    std::filesystem::path path_;
};

void writeFile(const std::filesystem::path& path, const std::string& content) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream output(path);
    if (!output)
        throw std::runtime_error("cannot create test file: " + path.string());
    output << content;
}

void addPackage(const std::filesystem::path& root, const std::string& name,
                const std::string& version,
                const std::vector<std::pair<std::string, std::string>>& dependencies = {},
                const std::string& nativeLibrary = {}) {
    const auto versionRoot = root / name / version;
    std::string manifest = "[package]\nname = \"" + name + "\"\nversion = \"" + version +
                           "\"\nsource = \"source.simp\"\nexport = \"namespace:Export\"\n";
    if (!dependencies.empty()) {
        manifest += "\n[dependencies]\n";
        for (const auto& dependency : dependencies) {
            manifest += dependency.first + " = \"=" + dependency.second + "\"\n";
        }
    }
    if (!nativeLibrary.empty()) {
        manifest +=
            "\n[link]\nlibraries = [\"" + nativeLibrary + "\"]\nlibrary-paths = [\"lib\"]\n";
    }
    writeFile(versionRoot / "simp-package.toml", manifest);
    writeFile(versionRoot / "source.simp", "namespace Export {\n}\n");
    if (!nativeLibrary.empty())
        std::filesystem::create_directories(versionRoot / "lib");
}

void writePolicy(const std::filesystem::path& path, const std::string& content) {
    writeFile(path, content);
}

simp::ModuleVersionPolicy readPolicy(const std::filesystem::path& path,
                                     const std::string& content) {
    writePolicy(path, content);
    const auto policy = simp::readModuleVersionPolicy(path);
    require(policy.has_value(), "expected policy file to be loaded");
    return *policy;
}

void expectFailure(const std::function<void()>& action, const std::string& message) {
    try {
        action();
    } catch (const std::runtime_error& error) {
        require(std::string(error.what()).find(message) != std::string::npos,
                "expected error containing '" + message + "', got: " + error.what());
        return;
    }
    throw std::runtime_error("expected failure containing: " + message);
}

const TestGroupRegistration registration{
    7,
    { { "modules.toml selects the first installed configured version",
        [] {
            TemporaryDirectory temporary;
            const auto root = temporary.path() / "packages";
            addPackage(root, "sample", "1.2.3");
            addPackage(root, "sample", "1.1.0");
            const auto policy = readPolicy(temporary.path() / "modules.toml",
                                           "[modules]\nsample = [\"1.2.3\", \"1.1.0\"]\n");
            const auto result = simp::resolvePackages({ "sample" }, { root }, policy);
            require(result.packages.at("sample").version == "1.2.3",
                    "the first installed configured version should win");
        } },
      { "selected package version carries its native link inputs",
        [] {
            TemporaryDirectory temporary;
            const auto root = temporary.path() / "packages";
            addPackage(root, "native", "1.0.0", {}, "old_native");
            addPackage(root, "native", "2.0.0", {}, "new_native");
            const auto policy = readPolicy(temporary.path() / "modules.toml",
                                           "[modules]\nnative = [\"2.0.0\", \"1.0.0\"]\n");
            const auto result = simp::resolvePackages({ "native" }, { root }, policy);
            const auto& package = result.packages.at("native");
            require(package.version == "2.0.0" &&
                        package.libraries == std::vector<std::string>{ "new_native" } &&
                        package.libraryPaths ==
                            std::vector<std::filesystem::path>{
                                root / "native" / "2.0.0" / "lib"} &&
                        result.linkOrder.size() == 1 &&
                        result.linkOrder.front().version == "2.0.0" &&
                        result.linkOrder.front().libraries == package.libraries,
                    "link inputs must come from the selected package version");
        } },
      { "modules.toml falls back only when an earlier version is absent",
        [] {
            TemporaryDirectory temporary;
            const auto root = temporary.path() / "packages";
            addPackage(root, "sample", "1.1.0");
            const auto policy = readPolicy(temporary.path() / "modules.toml",
                                           "[modules]\nsample = [\"1.2.3\", \"1.1.0\"]\n");
            const auto result = simp::resolvePackages({ "sample" }, { root }, policy);
            require(result.packages.at("sample").version == "1.1.0",
                    "selection should advance past an absent configured version");
        } },
      { "modules.toml reports all configured versions when none are installed",
        [] {
            TemporaryDirectory temporary;
            const auto root = temporary.path() / "packages";
            const auto policy = readPolicy(temporary.path() / "modules.toml",
                                           "[modules]\nsample = [\"2.0.0\", \"1.0.0\"]\n");
            expectFailure([&] { simp::resolvePackages({ "sample" }, { root }, policy); },
                          "versions searched in order: [2.0.0, 1.0.0]");
        } },
      { "modules.toml is an allowlist for direct and transitive packages",
        [] {
            TemporaryDirectory temporary;
            const auto root = temporary.path() / "packages";
            addPackage(root, "parent", "1.0.0", { { "child", "1.0.0" } });
            addPackage(root, "child", "1.0.0");
            const auto policy =
                readPolicy(temporary.path() / "modules.toml", "[modules]\nparent = [\"1.0.0\"]\n");
            expectFailure([&] { simp::resolvePackages({ "parent" }, { root }, policy); },
                          "module 'child' is not found");
            expectFailure([&] { simp::resolvePackages({ "child" }, { root }, policy); },
                          "module 'child' is not found");
        } },
      { "modules.toml also constrains packages in lower-priority standard roots",
        [] {
            TemporaryDirectory temporary;
            const auto projectRoot = temporary.path() / "project";
            const auto standardRoot = temporary.path() / "standard";
            addPackage(standardRoot, "bundled", "0.1.0");
            const auto policy =
                readPolicy(temporary.path() / "modules.toml", "[modules]\nbundled = [\"0.1.0\"]\n");
            const auto result =
                simp::resolvePackages({ "bundled" }, { projectRoot, standardRoot }, policy);
            require(result.packages.at("bundled").version == "0.1.0",
                    "a listed bundled package should be selectable");
            const auto emptyPolicy = readPolicy(temporary.path() / "empty.toml", "[modules]\n");
            expectFailure(
                [&] {
                    simp::resolvePackages({ "bundled" }, { projectRoot, standardRoot },
                                          emptyPolicy);
                },
                "module 'bundled' is not found");
        } },
      { "modules.toml selection stays within the first package root",
        [] {
            TemporaryDirectory temporary;
            const auto projectRoot = temporary.path() / "project";
            const auto standardRoot = temporary.path() / "standard";
            addPackage(projectRoot, "shadowed", "1.0.0");
            addPackage(standardRoot, "shadowed", "2.0.0");
            const auto policy = readPolicy(temporary.path() / "modules.toml",
                                           "[modules]\nshadowed = [\"2.0.0\", \"1.0.0\"]\n");
            const auto result =
                simp::resolvePackages({ "shadowed" }, { projectRoot, standardRoot }, policy);
            require(result.packages.at("shadowed").version == "1.0.0",
                    "a lower-priority root must not supply a shadowed version");
        } },
      { "exact dependency pins agree with configured selection or report conflict",
        [] {
            TemporaryDirectory temporary;
            const auto root = temporary.path() / "packages";
            addPackage(root, "parent", "1.0.0", { { "child", "1.0.0" } });
            addPackage(root, "child", "1.0.0");
            addPackage(root, "child", "2.0.0");
            const auto matching =
                readPolicy(temporary.path() / "matching.toml",
                           "[modules]\nparent = [\"1.0.0\"]\nchild = [\"1.0.0\", \"2.0.0\"]\n");
            auto result = simp::resolvePackages({ "parent" }, { root }, matching);
            require(result.packages.at("child").version == "1.0.0",
                    "an exact pin matching the first configured version should pass");

            const auto conflicting =
                readPolicy(temporary.path() / "conflicting.toml",
                           "[modules]\nparent = [\"1.0.0\"]\nchild = [\"2.0.0\", \"1.0.0\"]\n");
            expectFailure([&] { simp::resolvePackages({ "parent" }, { root }, conflicting); },
                          "modules.toml selects version '2.0.0'");
        } },
      { "modules.toml accepts exact prerelease SemVer versions",
        [] {
            TemporaryDirectory temporary;
            const auto root = temporary.path() / "packages";
            addPackage(root, "preview", "2.0.0-rc.1");
            const auto policy = readPolicy(temporary.path() / "modules.toml",
                                           "[modules]\npreview = [\"2.0.0-rc.1\"]\n");
            const auto result = simp::resolvePackages({ "preview" }, { root }, policy);
            require(result.packages.at("preview").version == "2.0.0-rc.1",
                    "an exact prerelease should be selectable");
        } },
      { "modules.toml rejects deprecated registry-only package resolution",
        [] {
            TemporaryDirectory temporary;
            const auto root = temporary.path() / "packages";
            const auto policy =
                readPolicy(temporary.path() / "modules.toml", "[modules]\nlegacy = [\"1.0.0\"]\n");
            expectFailure(
                [&] { simp::resolvePackages({ "legacy" }, { root }, policy, { "legacy" }); },
                "available only through the deprecated registry");
        } },
      { "absent modules.toml retains the legacy highest stable selection",
        [] {
            TemporaryDirectory temporary;
            const auto root = temporary.path() / "packages";
            addPackage(root, "sample", "1.0.0");
            addPackage(root, "sample", "2.0.0-rc.1");
            addPackage(root, "sample", "1.5.0");
            const auto policy = simp::readModuleVersionPolicy(temporary.path() / "missing.toml");
            require(!policy, "an absent policy should retain legacy resolution");
            const auto result = simp::resolvePackages({ "sample" }, { root }, policy);
            require(result.packages.at("sample").version == "1.5.0",
                    "legacy behavior should choose the highest stable version");
        } },
      { "modules.toml rejects malformed TOML and unsupported config types",
        [] {
            TemporaryDirectory temporary;
            const auto path = temporary.path() / "modules.toml";
            struct InvalidPolicy {
                std::string description;
                std::string content;
                std::string expectedError;
            };
            const std::vector<InvalidPolicy> invalid{
                { "missing table", "sample = [\"1.0.0\"]\n", "expected a module version array" },
                { "wrong value type", "[modules]\nsample = \"1.0.0\"\n", "quoted string" },
                { "invalid SemVer", "[modules]\nsample = [\"v1.0\"]\n", "invalid exact SemVer" },
                { "empty array", "[modules]\nsample = []\n", "nonempty version array" },
                { "duplicate version", "[modules]\nsample = [\"1.0.0\", \"1.0.0\"]\n",
                  "repeats version" },
                { "duplicate key", "[modules]\nsample = [\"1.0.0\"]\nsample = [\"2.0.0\"]\n",
                  "duplicate module key" },
                { "duplicate section", "[modules]\n[modules]\n", "duplicate table [modules]" },
                { "unsupported table", "[other]\n", "unsupported table" },
                { "non-array item", "[modules]\nsample = [\"1.0.0\", 2]\n", "quoted string" },
                { "multiline array", "[modules]\nsample = [\n  \"1.0.0\"\n]\n",
                  "arrays must be single-line" },
                { "invalid module name", "[modules]\n\"not a name\" = [\"1.0.0\"]\n",
                  "module names must be Simple identifiers" }
            };
            for (const auto& item : invalid) {
                writeFile(path, item.content);
                expectFailure([&] { static_cast<void>(simp::readModuleVersionPolicy(path)); },
                              item.expectedError);
            }
        } },
      { "invalid installed manifests do not trigger selection fallback",
        [] {
            TemporaryDirectory temporary;
            const auto root = temporary.path() / "packages";
            addPackage(root, "sample", "1.0.0");
            const auto invalidRoot = root / "sample" / "2.0.0";
            writeFile(invalidRoot / "simp-package.toml",
                      "[package]\nname = \"sample\"\nversion = \"2.0.0\"\n"
                      "source = \"source.simp\"\nexport = \"namespace:Export\"\n"
                      "unknown = \"bad\"\n");
            writeFile(invalidRoot / "source.simp", "namespace Export {\n}\n");
            const auto policy = readPolicy(temporary.path() / "modules.toml",
                                           "[modules]\nsample = [\"2.0.0\", \"1.0.0\"]\n");
            expectFailure([&] { simp::resolvePackages({ "sample" }, { root }, policy); },
                          "unknown key 'unknown' in [package]");
        } } }
};

} // namespace
