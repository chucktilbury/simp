#include "simp/CommandLine.hpp"

#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

void expectFailure(const std::function<void()>& action, const std::string& expected) {
    try {
        action();
    } catch (const std::invalid_argument& error) {
        require(std::string(error.what()).find(expected) != std::string::npos,
                "expected error containing '" + expected + "', got: " + error.what());
        return;
    }
    throw std::runtime_error("expected command-line parse failure: " + expected);
}

simp::CommandLine makeCommandLine() {
    simp::CommandLine commandLine("test", "test options", "1.2");
    simp::CommandLineOption alpha;
    alpha.shortName = 'a';
    alpha.longName = "alpha";
    alpha.name = "alpha";
    commandLine.addOption(alpha);

    simp::CommandLineOption beta;
    beta.shortName = 'b';
    beta.longName = "beta";
    beta.name = "beta";
    commandLine.addOption(beta);

    simp::CommandLineOption path;
    path.shortName = 'p';
    path.longName = "path";
    path.name = "path";
    path.valueType = simp::CommandLineValueType::String;
    path.list = true;
    commandLine.addOption(path);

    simp::CommandLineOption output;
    output.shortName = 'o';
    output.longName = "output";
    output.name = "output";
    output.valueType = simp::CommandLineValueType::String;
    output.defaultValue = "default.out";
    commandLine.addOption(output);

    simp::CommandLineOption count;
    count.longName = "count";
    count.name = "count";
    count.valueType = simp::CommandLineValueType::Number;
    count.required = true;
    commandLine.addOption(count);

    simp::CommandLineOption help;
    help.longName = "help";
    help.name = "help";
    help.action = simp::CommandLineAction::Help;
    commandLine.addOption(help);
    commandLine.addPositional({"source", "source file", true, true});
    return commandLine;
}

void testShortGroupsAttachedValuesAndLists() {
    auto commandLine = makeCommandLine();
    commandLine.parse({"-ab", "-p", "one:two", "-pthree", "-o=first",
                       "--output=last", "--count", "-5", "first.simp",
                       "--", "-input.simp"});
    require(commandLine.switchValue("alpha") && commandLine.switchValue("beta"),
            "short option groups should set each switch");
    require(commandLine.values("path") ==
                std::vector<std::string>{"one", "two", "three"},
            "repeated list options and colon-separated values should accumulate");
    require(commandLine.value("output") == std::optional<std::string>("last"),
            "the last scalar option should replace previous values");
    require(commandLine.wasProvided("output"),
            "overridden default values should be marked as explicitly provided");
    require(commandLine.value("count") == std::optional<std::string>("-5"),
            "separate option values may begin with a dash");
    require(commandLine.positionalValues() ==
                std::vector<std::string>{"first.simp", "-input.simp"},
            "list positionals should preserve each complete path");
}

void testDefaultsAndActions() {
    auto commandLine = makeCommandLine();
    commandLine.parse({"--help"});
    require(commandLine.action() == simp::CommandLineAction::Help,
            "help should stop parsing before required values are checked");
    require(commandLine.value("output") == std::optional<std::string>("default.out"),
            "registered defaults should be available after parsing");
    require(!commandLine.wasProvided("output"),
            "defaults should not count as command-line occurrences");
    require(commandLine.helpText().find("--count <value>") != std::string::npos,
            "help should show value-taking options");
    require(commandLine.versionText() == "test: v1.2\n",
            "version text should include the registered application version");
}

void testInvalidArguments() {
    auto commandLine = makeCommandLine();
    expectFailure([&] { commandLine.parse({"source.simp"}); },
                  "required option");
    commandLine.parse({"--count", "1", "source.simp", "other.simp"});
    require(commandLine.positionalValues() ==
                std::vector<std::string>{"source.simp", "other.simp"},
            "list positionals should accept multiple input files");
    expectFailure([&] { commandLine.parse({"--unknown", "--count", "1", "x"}); },
                  "unknown option");
    expectFailure([&] { commandLine.parse({"--output"}); }, "requires an argument");
    expectFailure([&] { commandLine.parse({"--alpha=yes", "--count", "1", "x"}); },
                  "does not accept an argument");
    expectFailure([&] { commandLine.parse({"--count", "many", "x"}); },
                  "requires a number");
}

void testCountersAndListSeparators() {
    simp::CommandLine commandLine("test", "test options", "1.2");
    simp::CommandLineOption verbose;
    verbose.shortName = 'v';
    verbose.name = "verbose";
    verbose.valueType = simp::CommandLineValueType::Counter;
    commandLine.addOption(verbose);
    simp::CommandLineOption quiet;
    quiet.shortName = 'q';
    quiet.name = "quiet";
    commandLine.addOption(quiet);
    simp::CommandLineOption trace;
    trace.shortName = 't';
    trace.longName = "trace";
    trace.name = "trace";
    trace.valueType = simp::CommandLineValueType::String;
    trace.list = true;
    trace.listSeparator = ',';
    commandLine.addOption(trace);

    commandLine.parse({"-vv", "-qv", "-t", "a,b", "--trace=c:d", "-te"});
    require(commandLine.count("verbose") == 3,
            "counted switches should count grouped and repeated occurrences");
    require(commandLine.switchValue("quiet"), "counters should combine with switches");
    require(commandLine.values("trace") == std::vector<std::string>{"a", "b", "c:d", "e"},
            "list options should split only on their own separator");
    require(commandLine.helpText().find("(repeatable)") != std::string::npos,
            "help should mark counted switches as repeatable");
    commandLine.parse({});
    require(commandLine.count("verbose") == 0, "counts should reset between parses");
    expectFailure([&] { commandLine.parse({"-v=2"}); }, "does not accept an argument");

    simp::CommandLine invalid("test", "test options", "1.2");
    simp::CommandLineOption listCounter;
    listCounter.shortName = 'c';
    listCounter.name = "count";
    listCounter.valueType = simp::CommandLineValueType::Counter;
    listCounter.list = true;
    expectFailure([&] { invalid.addOption(listCounter); }, "list");
}

} // namespace

int main() {
    const std::vector<std::pair<std::string, std::function<void()>>> tests = {
        {"short groups, attached values, and lists", testShortGroupsAttachedValuesAndLists},
        {"defaults and actions", testDefaultsAndActions},
        {"invalid arguments", testInvalidArguments},
        {"counters and list separators", testCountersAndListSeparators},
    };
    for (const auto& test : tests) {
        try {
            test.second();
        } catch (const std::exception& error) {
            std::cerr << "FAIL " << test.first << ": " << error.what() << '\n';
            return 1;
        }
    }
    std::cout << "All command-line parser tests passed\n";
    return 0;
}
