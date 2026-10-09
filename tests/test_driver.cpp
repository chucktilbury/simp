#include "json_lite.hpp"
#include "test_cases.hpp"

#include "cwhip/Diagnostic.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

struct FunctionalCase {
    std::string friendlyName;
    std::string fixture;
    std::string expectedDiagnostic;
    bool enabled = true;
};

struct RunEntry {
    std::string name;
    std::function<void()> action;
    bool skip = false;
};

std::string readFile(const std::filesystem::path& path) {
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("cannot read test file: " + path.string());
    }
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

void runFunctional(const std::string& filename, const std::string& expectedError) {
    const auto path = std::filesystem::path(CWHIP_TEST_SOURCE_DIR) / filename;
    try {
        cwhip_test::parse(readFile(path), path.string());
    } catch (const cwhip::DiagnosticError& error) {
        if (expectedError.empty()) {
            throw;
        }
        cwhip_test::require(std::string(error.what()).find(expectedError) != std::string::npos,
                           "unexpected diagnostic: " + std::string(error.what()));
        return;
    }
    cwhip_test::require(expectedError.empty(), "expected functional program to fail: " + filename);
}

FunctionalCase loadFunctionalCase(const std::filesystem::path& metadata,
                                  const std::filesystem::path& root) {
    auto fixture = metadata;
    fixture.replace_extension();
    if (!std::filesystem::is_regular_file(fixture) || fixture.extension() != ".cw") {
        throw std::runtime_error(metadata.string() + ": missing matching .cw fixture");
    }

    cwhip_test_json::JsonValue entry;
    try {
        entry = cwhip_test_json::JsonParser(readFile(metadata)).parse();
        if (entry.type != cwhip_test_json::JsonType::Object) {
            throw std::runtime_error("expected a JSON object");
        }
        const auto& friendlyName = entry.at("friendly_name");
        const auto& expectedDiagnostic = entry.at("expected_diagnostic");
        const auto& enabled = entry.at("enabled");
        if (friendlyName.type != cwhip_test_json::JsonType::String ||
            expectedDiagnostic.type != cwhip_test_json::JsonType::String ||
            enabled.type != cwhip_test_json::JsonType::Boolean) {
            throw std::runtime_error("field has the wrong JSON type");
        }
        return {friendlyName.stringValue, fixture.lexically_relative(root).generic_string(),
                expectedDiagnostic.stringValue, enabled.boolValue};
    } catch (const std::runtime_error& error) {
        throw std::runtime_error(metadata.string() + ": " + error.what());
    }
}

std::vector<FunctionalCase> loadFunctionalCases(const std::filesystem::path& root) {
    std::vector<std::filesystem::path> metadata;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(root)) {
        const auto filename = entry.path().filename().string();
        const std::string suffix = ".cw.json";
        if (entry.is_regular_file() && filename.size() >= suffix.size() &&
            filename.compare(filename.size() - suffix.size(), suffix.size(), suffix) == 0) {
            metadata.push_back(entry.path());
        }
    }
    std::sort(metadata.begin(), metadata.end());
    std::vector<FunctionalCase> cases;
    cases.reserve(metadata.size());
    for (const auto& path : metadata) {
        cases.push_back(loadFunctionalCase(path, root));
    }
    return cases;
}

} // namespace

int main() {
    const auto tests = cwhip_test::registeredTests();
    const auto functionalCases = loadFunctionalCases(CWHIP_TEST_SOURCE_DIR);

    std::vector<RunEntry> runEntries;
    runEntries.reserve(tests.size() + functionalCases.size());
    for (const auto& test : tests) {
        runEntries.push_back({test.first, test.second, false});
    }
    for (const auto& functionalCase : functionalCases) {
        if (functionalCase.enabled) {
            runEntries.push_back(
                {functionalCase.friendlyName,
                 [functionalCase] {
                     runFunctional(functionalCase.fixture, functionalCase.expectedDiagnostic);
                 },
                 false});
        } else {
            runEntries.push_back({functionalCase.friendlyName, nullptr, true});
        }
    }

    std::size_t failures = 0;
    std::size_t skipped = 0;
    for (const auto& entry : runEntries) {
        if (entry.skip) {
            ++skipped;
            std::cout << "SKIP " << entry.name << " (disabled)\n";
            continue;
        }
        try {
            entry.action();
            std::cout << "PASS " << entry.name << '\n';
        } catch (const std::exception& error) {
            ++failures;
            std::cerr << "FAIL " << entry.name << ": " << error.what() << '\n';
        }
    }
    const std::size_t total = runEntries.size() - skipped;
    std::cout << total - failures << "/" << total << " tests passed";
    if (skipped > 0) {
        std::cout << " (" << skipped << " skipped)";
    }
    std::cout << '\n';
    return failures == 0 ? 0 : 1;
}
