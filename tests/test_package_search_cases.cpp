#include "test_cases.hpp"

#include "simp/PackageIntegrity.hpp"
#include "simp/PackageRegistry.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

namespace {
using namespace simp_test;

class PackageRoots {
public:
    PackageRoots()
        : root(std::filesystem::temp_directory_path() /
               ("simp-package-root-tests-" +
                std::to_string(std::chrono::steady_clock::now()
                                   .time_since_epoch().count()))) {
        std::filesystem::create_directories(root);
    }
    ~PackageRoots() {
        std::error_code error;
        std::filesystem::remove_all(root, error);
    }
    std::filesystem::path package(const std::filesystem::path& moduleRoot,
                                  const std::string& name,
                                  const std::string& version,
                                  const std::string& source) const {
        const auto packageRoot = moduleRoot / name / version;
        std::filesystem::create_directories(packageRoot);
        std::ofstream(packageRoot / "simp-package.toml")
            << "[package]\nname = \"" << name << "\"\nversion = \"" << version
            << "\"\nsource = \"source.simp\"\nexport = \"namespace:Export\"\n";
        std::ofstream(packageRoot / "source.simp") << source;
        return packageRoot;
    }
    std::filesystem::path root;
};

const TestGroupRegistration registration{7, {
    {"first package root wins a same-version collision", [] {
        PackageRoots fixture;
        const auto first = fixture.root / "cli";
        const auto second = fixture.root / "project";
        const auto selected = fixture.package(first, "sample", "1.0.0",
                                               "namespace Export { int first() { return 1 } }\n");
        fixture.package(second, "sample", "1.0.0",
                        "namespace Export { int second() { return 2 } }\n");
        const auto result = simp::resolvePackages({"sample"}, {first, second});
        require(result.packages.at("sample").packageRoot == selected,
                "the first matching package root must shadow lower-priority roots");
    }},
    {"invalid higher-priority package candidates do not fall through", [] {
        PackageRoots fixture;
        const auto first = fixture.root / "home";
        const auto second = fixture.root / "installation";
        const auto invalid = first / "sample" / "1.0.0";
        std::filesystem::create_directories(invalid);
        std::ofstream(invalid / "source.simp") << "namespace Export {}\n";
        fixture.package(second, "sample", "1.0.0", "namespace Export {}\n");
        try {
            static_cast<void>(simp::resolvePackages({"sample"}, {first, second}));
        } catch (const std::runtime_error& error) {
            require(std::string(error.what()).find("missing simp-package.toml") !=
                        std::string::npos,
                    "invalid candidate diagnostics should identify the missing manifest");
            return;
        }
        throw std::runtime_error("expected invalid higher-priority candidate to fail");
    }},
    {"locked package hashes reject a conflicting higher-priority override", [] {
        PackageRoots fixture;
        const auto overrideRoot = fixture.root / "home";
        const auto installationRoot = fixture.root / "installation";
        const auto lockedPackage = fixture.package(
            installationRoot, "sample", "1.0.0", "namespace Export {}\n");
        fixture.package(overrideRoot, "sample", "1.0.0",
                        "namespace Export { int changed() { return 1 } }\n");

        simp::ModuleVersionPolicy policy;
        policy.path = fixture.root / "simpkg.lock";
        policy.versions["sample"] = {"1.0.0"};
        policy.lockedPackages["sample"]["sha256"] =
            simp::packageTreeSha256(lockedPackage);
        policy.lockedDependencies["sample"] = {};
        try {
            static_cast<void>(
                simp::resolvePackages({"sample"}, {overrideRoot, installationRoot}, policy));
        } catch (const std::runtime_error& error) {
            require(std::string(error.what()).find("package integrity mismatch") !=
                        std::string::npos,
                    "the locked hash must reject a modified home override");
            return;
        }
        throw std::runtime_error("expected a modified locked override to fail");
    }},
    {"a higher root with only another version cannot bypass a lock", [] {
        PackageRoots fixture;
        const auto overrideRoot = fixture.root / "environment";
        const auto installationRoot = fixture.root / "project";
        fixture.package(overrideRoot, "sample", "2.0.0", "namespace Export {}\n");
        const auto lockedPackage =
            fixture.package(installationRoot, "sample", "1.0.0", "namespace Export {}\n");

        simp::ModuleVersionPolicy policy;
        policy.path = fixture.root / "simpkg.lock";
        policy.versions["sample"] = {"1.0.0"};
        policy.lockedPackages["sample"]["sha256"] =
            simp::packageTreeSha256(lockedPackage);
        policy.lockedDependencies["sample"] = {};
        try {
            static_cast<void>(
                simp::resolvePackages({"sample"}, {overrideRoot, installationRoot}, policy));
        } catch (const std::runtime_error& error) {
            require(std::string(error.what()).find("no installed configured version") !=
                        std::string::npos,
                    "the lock should report the pinned version missing from the first root");
            return;
        }
        throw std::runtime_error("expected the higher root's version conflict to fail");
    }},
}};

} // namespace
