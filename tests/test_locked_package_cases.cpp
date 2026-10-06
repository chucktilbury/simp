#include "test_cases.hpp"
#include "simp/PackageIntegrity.hpp"
#include "simp/PackageRegistry.hpp"
#include "simp/PathResolution.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>

namespace {
using namespace simp_test;

class Project {
public:
    Project() {
        root = std::filesystem::temp_directory_path() /
            ("simp-lock-tests-" +
             std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        std::filesystem::create_directories(root / "modules/sample/1.0.0");
        std::filesystem::create_directories(root / "src/nested");
        write(root / "simpkg.toml",
              "schema = 1\n[dependencies.sample]\nrepo = \"acme/sample\"\nversion = \"=1.0.0\"\n");
        write(package() / "simp-package.toml",
              "[package]\nname = \"sample\"\nversion = \"1.0.0\"\n"
              "source = \"source.simp\"\nexport = \"namespace:Export\"\n");
        write(package() / "source.simp", "namespace Export {}\n");
        lock();
    }
    ~Project() {
        std::error_code error;
        std::filesystem::remove_all(root, error);
    }
    std::filesystem::path package() const { return root / "modules/sample/1.0.0"; }
    static void write(const std::filesystem::path& path, const std::string& value) {
        std::ofstream output(path, std::ios::binary);
        if (!output) throw std::runtime_error("cannot write test file");
        output << value;
    }
    void lock(const std::string& extra = {}) const {
        write(root / "simpkg.lock",
              "schema = 1\nmanifest-sha256 = \"" +
              simp::packageFileSha256(root / "simpkg.toml") +
              "\"\n[modules]\nsample = [\"1.0.0\"]\n[packages.sample]\n"
              "repo = \"acme/sample\"\nversion = \"1.0.0\"\nref = \"refs/tags/1.0.0\"\n"
              "commit = \"0123456789012345678901234567890123456789\"\nsha256 = \"" +
              simp::packageTreeSha256(package()) + "\"\ndependencies = []\n" + extra);
    }
    simp::ModuleSearchPaths paths(
        const std::optional<std::string>& moduleOption = std::nullopt,
        const std::optional<std::string>& moduleEnvironment = std::nullopt) const {
        simp::ModuleSearchRequest request;
        request.sourcePaths = {"src/nested/main.simp"};
        request.currentDirectory = root;
        request.moduleDirectoryOption = moduleOption;
        simp::ResourcePaths resources;
        resources.standardModuleDirectory = {root / "stdlib", "test"};
        return simp::resolveModuleSearchPaths(
            request, resources,
            [moduleEnvironment](const std::string& name) -> std::optional<std::string> {
                return name == "SIMP_MODULE_DIR" ? moduleEnvironment : std::nullopt;
            });
    }
    std::filesystem::path root;
};

void failure(const std::function<void()>& action, const std::string& expected) {
    try {
        action();
    } catch (const std::runtime_error& error) {
        require(std::string(error.what()).find(expected) != std::string::npos,
                "expected '" + expected + "', got: " + error.what());
        return;
    }
    throw std::runtime_error("expected error: " + expected);
}

const TestGroupRegistration registration{8, {
    {"package SHA256 matches standard empty and multiblock test vectors", [] {
        Project project;
        Project::write(project.root / "hash", "");
        require(simp::packageFileSha256(project.root / "hash") ==
                    "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855",
                "empty SHA256 mismatch");
        Project::write(project.root / "hash",
                       "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq");
        require(simp::packageFileSha256(project.root / "hash") ==
                    "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1",
                "padded multiblock SHA256 mismatch");
    }},
    {"nested source paths discover the nearest locked project without activation", [] {
        Project project;
        const auto paths = project.paths();
        require(paths.projectRoot.path == project.root &&
                    paths.projectModuleRoot.path == project.root / "modules" &&
                    paths.projectLockFile.path == project.root / "simpkg.lock",
                "nested source did not discover the project lock");
        const auto policy = simp::readModuleVersionPolicy(paths.projectLockFile.path);
        const auto graph = simp::resolvePackages({"sample"}, {project.root / "modules"}, policy);
        require(graph.packages.at("sample").version == "1.0.0",
                "compiler did not use the exact locked version");
    }},
    {"storage overrides do not relocate project lock discovery", [] {
        Project project;
        const auto option = project.paths("alternate", "/environment");
        require(option.commandLineModuleRoot &&
                    option.commandLineModuleRoot->path == project.root / "alternate" &&
                    option.projectLockFile.path == project.root / "simpkg.lock",
                "-M must add a higher-priority storage root without moving the lock");
        const auto environment = project.paths(std::nullopt, "/environment");
        require(environment.environmentModuleRoot &&
                    environment.environmentModuleRoot->path == "/environment" &&
                    environment.projectLockFile.path == project.root / "simpkg.lock",
                "environment roots must not move project configuration");
        const auto roots = option.packageRoots();
        require(roots.size() >= 3 && roots[0].path == project.root / "alternate" &&
                    roots[1].path == project.root / "modules" &&
                    roots[2].path == "/environment",
                "package roots should put CLI before project before environment");
    }},
    {"legacy module policies and missing locks fail with migration diagnostics", [] {
        Project project;
        Project::write(project.root / "modules/modules.toml", "[modules]\n");
        failure([&] { project.paths(); }, "legacy modules/modules.toml is no longer supported");
        std::filesystem::remove(project.root / "modules/modules.toml");
        std::filesystem::remove(project.root / "simpkg.lock");
        failure([&] { project.paths(); }, "missing simpkg.lock");
    }},
    {"compiler rejects stale manifests and invalid lock schemas", [] {
        Project project;
        Project::write(project.root / "simpkg.toml", "schema = 1\n");
        failure([&] { simp::readModuleVersionPolicy(project.root / "simpkg.lock"); },
                "stale lockfile");
        Project::write(project.root / "simpkg.lock", "schema = 2\n");
        failure([&] { simp::readModuleVersionPolicy(project.root / "simpkg.lock"); },
                "unsupported lock schema");
    }},
    {"compiler refuses to compile modified locked package contents", [] {
        Project project;
        const auto policy = simp::readModuleVersionPolicy(project.root / "simpkg.lock");
        Project::write(project.package() / "source.simp", "namespace Changed {}\n");
        failure([&] { simp::resolvePackages({"sample"}, {project.root / "modules"}, policy); },
                "package integrity mismatch");
    }},
    {"locked package trees reject symlinks", [] {
        Project project;
        std::filesystem::create_symlink(project.root / "simpkg.toml",
                                       project.package() / "escape");
        failure([&] { simp::packageTreeSha256(project.package()); },
                "rejects symlinks");
    }},
    {"lock parser rejects unknown metadata and incomplete graph nodes", [] {
        Project project;
        project.lock("unknown = \"value\"\n");
        failure([&] { simp::readModuleVersionPolicy(project.root / "simpkg.lock"); },
                "unknown locked package key");
        project.lock("\n[packages.extra]\nversion = \"1.0.0\"\n");
        failure([&] { simp::readModuleVersionPolicy(project.root / "simpkg.lock"); },
                "graph does not match");
    }},
    {"lock parser rejects fallback arrays, inconsistent edges, and dependency cycles", [] {
        Project project;
        const auto replaceLock = [&](const std::string& before, const std::string& after) {
            project.lock();
            std::ifstream input(project.root / "simpkg.lock");
            std::string content{std::istreambuf_iterator<char>(input),
                                std::istreambuf_iterator<char>()};
            const auto position = content.find(before);
            require(position != std::string::npos, "missing lock test replacement");
            content.replace(position, before.size(), after);
            Project::write(project.root / "simpkg.lock", content);
        };
        replaceLock("sample = [\"1.0.0\"]", "sample = [\"1.0.0\", \"2.0.0\"]");
        failure([&] { simp::readModuleVersionPolicy(project.root / "simpkg.lock"); },
                "exactly one exact SemVer");
        replaceLock("dependencies = []", "dependencies = [\"sample=2.0.0\"]");
        failure([&] { simp::readModuleVersionPolicy(project.root / "simpkg.lock"); },
                "conflicting locked dependency");
        replaceLock("dependencies = []", "dependencies = [\"sample=1.0.0\"]");
        failure([&] { simp::readModuleVersionPolicy(project.root / "simpkg.lock"); },
                "cyclic locked dependency");
    }},
    {"package dependency source mapping is recognized and checked by the compiler", [] {
        Project project;
        const auto dependency = project.root / "modules/child/1.0.0";
        std::filesystem::create_directories(dependency);
        Project::write(dependency / "simp-package.toml",
              "[package]\nname = \"child\"\nversion = \"1.0.0\"\n"
              "source = \"source.simp\"\nexport = \"namespace:Child\"\n");
        Project::write(dependency / "source.simp", "namespace Child {}\n");
        Project::write(project.package() / "simp-package.toml",
              "[package]\nname = \"sample\"\nversion = \"1.0.0\"\n"
              "source = \"source.simp\"\nexport = \"namespace:Export\"\n"
              "[dependencies]\nchild = \"=1.0.0\"\n"
              "[sources]\nchild = \"acme/child\"\n");
        const auto graph = simp::resolvePackages({"sample"}, {project.root / "modules"});
        require(graph.packages.size() == 2 &&
                    graph.packages.at("child").version == "1.0.0",
                "valid source mapping must not change package dependency semantics");
        Project::write(project.package() / "simp-package.toml",
              "[package]\nname = \"sample\"\nversion = \"1.0.0\"\n"
              "source = \"source.simp\"\nexport = \"namespace:Export\"\n"
              "[sources]\nunknown = \"acme/unknown\"\n");
        failure([&] { simp::resolvePackages({"sample"}, {project.root / "modules"}); },
                "map declared dependencies");
    }},
}};

} // namespace
