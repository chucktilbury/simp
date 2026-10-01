/**
 * @file CommandLine.hpp
 * @brief Registration-based command-line option parsing.
 */
#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace simp {

enum class CommandLineValueType { Switch, Counter, String, Number };
enum class CommandLineAction { None, Help, Version };

struct CommandLineOption {
    char shortName = '\0';
    std::string longName;
    std::string name;
    std::string description;
    CommandLineValueType valueType = CommandLineValueType::Switch;
    bool list = false;
    char listSeparator = ':';
    bool required = false;
    std::optional<std::string> defaultValue;
    CommandLineAction action = CommandLineAction::None;
};

struct CommandLinePositional {
    std::string name;
    std::string description;
    bool required = false;
    bool list = false;
};

class CommandLine {
public:
    CommandLine(std::string programName, std::string description, std::string version);

    void addOption(CommandLineOption option);
    void addPositional(CommandLinePositional positional);
    void parse(const std::vector<std::string>& arguments);

    bool wasProvided(const std::string& name) const;
    bool switchValue(const std::string& name) const;
    std::size_t count(const std::string& name) const;
    std::optional<std::string> value(const std::string& name) const;
    const std::vector<std::string>& values(const std::string& name) const;
    bool contains(const std::string& name, const std::string& item) const;
    const std::vector<std::string>& positionalValues() const noexcept;
    CommandLineAction action() const noexcept;

    std::string helpText() const;
    std::string versionText() const;

private:
    const CommandLineOption* findLongOption(const std::string& name) const;
    const CommandLineOption* findShortOption(char name) const;
    void recordValue(const CommandLineOption& option, const std::string& value,
                     bool provided);
    void recordPositional(const std::string& value);

    std::string programName_;
    std::string description_;
    std::string version_;
    std::vector<CommandLineOption> options_;
    std::optional<CommandLinePositional> positional_;
    std::unordered_map<std::string, std::vector<std::string>> values_;
    std::unordered_map<std::string, bool> provided_;
    std::unordered_map<std::string, std::size_t> counts_;
    std::vector<std::string> positionalValues_;
    CommandLineAction action_ = CommandLineAction::None;
};

} // namespace simp
