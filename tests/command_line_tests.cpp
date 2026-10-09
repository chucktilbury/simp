#include "cwhip/CommandLine.hpp"

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

cwhip::CommandLine makeCommandLine() {
    cwhip::CommandLine commandLine("test", "test options", "1.2");
    cwhip::CommandLineOption alpha;
    alpha.shortName = 'a';
    alpha.longName = "alpha";
    alpha.name = "alpha";
    commandLine.addOption(alpha);

    cwhip::CommandLineOption beta;
    beta.shortName = 'b';
    beta.longName = "beta";
    beta.name = "beta";
    commandLine.addOption(beta);

    cwhip::CommandLineOption path;
    path.shortName = 'p';
    path.longName = "path";
    path.name = "path";
    path.valueType = cwhip::CommandLineValueType::String;
    path.list = true;
    commandLine.addOption(path);

    cwhip::CommandLineOption output;
    output.shortName = 'o';
    output.longName = "output";
    output.name = "output";
    output.valueType = cwhip::CommandLineValueType::String;
    output.defaultValue = "default.out";
    commandLine.addOption(output);

    cwhip::CommandLineOption count;
    count.longName = "count";
    count.name = "count";
    count.valueType = cwhip::CommandLineValueType::Number;
    count.required = true;
    commandLine.addOption(count);

    cwhip::CommandLineOption help;
    help.longName = "help";
    help.name = "help";
    help.action = cwhip::CommandLineAction::Help;
    commandLine.addOption(help);
    commandLine.addPositional({"source", "source file", true, true});
    return commandLine;
}

void testShortGroupsAttachedValuesAndLists() {
    auto commandLine = makeCommandLine();
    commandLine.parse({"-ab", "-p", "one:two", "-pthree", "-o=first",
                       "--output=last", "--count", "-5", "first.cw",
                       "--", "-input.cw"});
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
                std::vector<std::string>{"first.cw", "-input.cw"},
            "list positionals should preserve each complete path");
}

void testDefaultsAndActions() {
    auto commandLine = makeCommandLine();
    commandLine.parse({"--help"});
    require(commandLine.action() == cwhip::CommandLineAction::Help,
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
    expectFailure([&] { commandLine.parse({"source.cw"}); },
                  "required option");
    commandLine.parse({"--count", "1", "source.cw", "other.cw"});
    require(commandLine.positionalValues() ==
                std::vector<std::string>{"source.cw", "other.cw"},
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
    cwhip::CommandLine commandLine("test", "test options", "1.2");
    cwhip::CommandLineOption verbose;
    verbose.shortName = 'v';
    verbose.name = "verbose";
    verbose.valueType = cwhip::CommandLineValueType::Counter;
    commandLine.addOption(verbose);
    cwhip::CommandLineOption quiet;
    quiet.shortName = 'q';
    quiet.name = "quiet";
    commandLine.addOption(quiet);
    cwhip::CommandLineOption trace;
    trace.shortName = 't';
    trace.longName = "trace";
    trace.name = "trace";
    trace.valueType = cwhip::CommandLineValueType::String;
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

    cwhip::CommandLine invalid("test", "test options", "1.2");
    cwhip::CommandLineOption listCounter;
    listCounter.shortName = 'c';
    listCounter.name = "count";
    listCounter.valueType = cwhip::CommandLineValueType::Counter;
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
