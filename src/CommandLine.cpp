#include "cwhip/CommandLine.hpp"

#include <algorithm>
#include <charconv>
#include <cstdint>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace cwhip {
namespace {

std::vector<std::string> splitListValue(const std::string& value, char separatorCharacter) {
    std::vector<std::string> result;
    std::size_t start = 0;
    while (start <= value.size()) {
        const auto separator = value.find(separatorCharacter, start);
        const auto end = separator == std::string::npos ? value.size() : separator;
        result.push_back(value.substr(start, end - start));
        if (separator == std::string::npos || separator + 1 == value.size()) break;
        start = separator + 1;
    }
    return result;
}

} // namespace

CommandLine::CommandLine(std::string programName, std::string description,
                         std::string version)
    : programName_(std::move(programName)), description_(std::move(description)),
      version_(std::move(version)) {}

void CommandLine::addOption(CommandLineOption option) {
    if (option.name.empty() || (option.longName.empty() && option.shortName == '\0')) {
        throw std::invalid_argument("command-line options need a value name and an option name");
    }
    const bool takesValue = option.valueType == CommandLineValueType::String ||
                            option.valueType == CommandLineValueType::Number;
    if (!takesValue && option.list) {
        throw std::invalid_argument("switch options cannot accept list values");
    }
    if (option.list && option.listSeparator == '\0') {
        throw std::invalid_argument("list options need a list separator");
    }
    const auto duplicateName =
        std::find_if(options_.begin(), options_.end(), [&option](const auto& current) {
            return current.name == option.name;
        });
    if (duplicateName != options_.end() || findLongOption(option.longName) != nullptr ||
        (option.shortName != '\0' && findShortOption(option.shortName) != nullptr)) {
        throw std::invalid_argument("duplicate command-line option");
    }
    options_.push_back(std::move(option));
}

void CommandLine::addPositional(CommandLinePositional positional) {
    if (positional.name.empty()) {
        throw std::invalid_argument("positional arguments need a value name");
    }
    if (positional_) {
        throw std::invalid_argument("only one positional argument is supported");
    }
    positional_ = std::move(positional);
}

const CommandLineOption* CommandLine::findLongOption(const std::string& name) const {
    const auto found = std::find_if(options_.begin(), options_.end(),
                                    [&name](const auto& option) {
                                        return !option.longName.empty() &&
                                               option.longName == name;
                                    });
    return found == options_.end() ? nullptr : &*found;
}

const CommandLineOption* CommandLine::findShortOption(char name) const {
    const auto found = std::find_if(options_.begin(), options_.end(),
                                    [name](const auto& option) {
                                        return option.shortName == name;
                                    });
    return found == options_.end() ? nullptr : &*found;
}

void CommandLine::recordValue(const CommandLineOption& option, const std::string& value,
                              bool provided) {
    auto& destination = values_[option.name];
    if (!option.list) destination.clear();
    auto items = option.list ? splitListValue(value, option.listSeparator)
                             : std::vector<std::string>{value};
    if (option.valueType == CommandLineValueType::Number) {
        for (const auto& item : items) {
            std::int64_t number = 0;
            const auto result =
                std::from_chars(item.data(), item.data() + item.size(), number);
            if (item.empty() || result.ec != std::errc{} ||
                result.ptr != item.data() + item.size()) {
                const auto displayName = option.longName.empty()
                                             ? std::string("-") + option.shortName
                                             : "--" + option.longName;
                throw std::invalid_argument("option " + displayName +
                                            " requires a number");
            }
        }
    }
    destination.insert(destination.end(), items.begin(), items.end());
    if (provided) provided_[option.name] = true;
}

void CommandLine::recordPositional(const std::string& value) {
    if (!positional_) {
        throw std::invalid_argument("unexpected positional argument: " + value);
    }
    if (positional_->list) {
        positionalValues_.push_back(value);
    } else if (positionalValues_.empty()) {
        positionalValues_.push_back(value);
    } else {
        throw std::invalid_argument("only one input file is supported");
    }
}

void CommandLine::parse(const std::vector<std::string>& arguments) {
    values_.clear();
    provided_.clear();
    counts_.clear();
    positionalValues_.clear();
    action_ = CommandLineAction::None;
    for (const auto& option : options_) {
        if (option.defaultValue) recordValue(option, *option.defaultValue, false);
    }

    const auto processOption = [this](const CommandLineOption& option,
                                      const std::optional<std::string>& attached,
                                      std::size_t& index,
                                      const std::vector<std::string>& args) {
        if (option.action != CommandLineAction::None) {
            if (attached) {
                const auto displayName = option.longName.empty()
                                             ? std::string("-") + option.shortName
                                             : "--" + option.longName;
                throw std::invalid_argument("option " + displayName +
                                            " does not accept an argument");
            }
            action_ = option.action;
            return true;
        }
        if (option.valueType == CommandLineValueType::Switch) {
            if (attached) {
                const auto displayName = option.longName.empty()
                                             ? std::string("-") + option.shortName
                                             : "--" + option.longName;
                throw std::invalid_argument("option " + displayName +
                                            " does not accept an argument");
            }
            provided_[option.name] = true;
            return false;
        }
        if (option.valueType == CommandLineValueType::Counter) {
            if (attached) {
                const auto displayName = option.longName.empty()
                                             ? std::string("-") + option.shortName
                                             : "--" + option.longName;
                throw std::invalid_argument("option " + displayName +
                                            " does not accept an argument");
            }
            provided_[option.name] = true;
            ++counts_[option.name];
            return false;
        }
        std::string argument;
        if (attached) {
            argument = *attached;
        } else {
            if (index + 1 >= args.size()) {
                const auto optionName = option.longName.empty()
                                            ? std::string("-") + option.shortName
                                            : "--" + option.longName;
                throw std::invalid_argument("option " + optionName +
                                            " requires an argument");
            }
            argument = args[++index];
        }
        if (argument.empty()) {
            const auto optionName = option.longName.empty()
                                        ? std::string("-") + option.shortName
                                        : "--" + option.longName;
            throw std::invalid_argument("option " + optionName +
                                        " requires a non-empty argument");
        }
        recordValue(option, argument, true);
        return false;
    };

    bool positionalOnly = false;
    for (std::size_t index = 0; index < arguments.size(); ++index) {
        const auto& argument = arguments[index];
        if (!positionalOnly && argument == "--") {
            positionalOnly = true;
            continue;
        }
        if (!positionalOnly && argument.size() > 1 && argument.rfind("--", 0) == 0) {
            const auto separator = argument.find('=');
            const auto name = argument.substr(2, separator == std::string::npos
                                                     ? std::string::npos
                                                     : separator - 2);
            const auto* option = findLongOption(name);
            if (option == nullptr) {
                throw std::invalid_argument("unknown option: --" + name);
            }
            const std::optional<std::string> attached =
                separator == std::string::npos
                    ? std::nullopt
                    : std::optional<std::string>(argument.substr(separator + 1));
            if (processOption(*option, attached, index, arguments)) return;
            continue;
        }
        if (!positionalOnly && argument.size() > 1 && argument[0] == '-') {
            for (std::size_t offset = 1; offset < argument.size(); ++offset) {
                const auto* option = findShortOption(argument[offset]);
                if (option == nullptr) {
                    throw std::invalid_argument(std::string("unknown option: -") +
                                                argument[offset]);
                }
                if ((option->valueType == CommandLineValueType::Switch ||
                     option->valueType == CommandLineValueType::Counter) &&
                    option->action == CommandLineAction::None) {
                    const bool attachedValue =
                        offset + 1 < argument.size() && argument[offset + 1] == '=';
                    const std::optional<std::string> attached =
                        attachedValue ? std::optional<std::string>(argument.substr(offset + 2))
                                      : std::nullopt;
                    if (processOption(*option, attached, index, arguments)) return;
                    if (attachedValue) break;
                    continue;
                }

                std::optional<std::string> attached;
                if (offset + 1 < argument.size()) {
                    auto valueStart = offset + 1;
                    if (argument[valueStart] == '=') ++valueStart;
                    if (valueStart < argument.size()) {
                        attached = argument.substr(valueStart);
                    } else {
                        attached = std::string();
                    }
                }
                if (processOption(*option, attached, index, arguments)) return;
                break;
            }
            continue;
        }
        recordPositional(argument);
    }

    for (const auto& option : options_) {
        if (option.required && !provided_[option.name]) {
            const auto displayName = option.longName.empty()
                                         ? std::string("-") + option.shortName
                                         : "--" + option.longName;
            throw std::invalid_argument("required option not found: " + displayName);
        }
    }
    if (positional_ && positional_->required && positionalValues_.empty()) {
        throw std::invalid_argument("required argument not found: " + positional_->name);
    }
}

bool CommandLine::wasProvided(const std::string& name) const {
    const auto found = provided_.find(name);
    return found != provided_.end() && found->second;
}

bool CommandLine::switchValue(const std::string& name) const {
    return wasProvided(name);
}

std::size_t CommandLine::count(const std::string& name) const {
    const auto found = counts_.find(name);
    return found == counts_.end() ? 0 : found->second;
}

std::optional<std::string> CommandLine::value(const std::string& name) const {
    const auto found = values_.find(name);
    if (found == values_.end() || found->second.empty()) return std::nullopt;
    return found->second.back();
}

const std::vector<std::string>& CommandLine::values(const std::string& name) const {
    static const std::vector<std::string> empty;
    const auto found = values_.find(name);
    return found == values_.end() ? empty : found->second;
}

bool CommandLine::contains(const std::string& name, const std::string& item) const {
    const auto& items = values(name);
    return std::find(items.begin(), items.end(), item) != items.end();
}

const std::vector<std::string>& CommandLine::positionalValues() const noexcept {
    return positionalValues_;
}

CommandLineAction CommandLine::action() const noexcept {
    return action_;
}

std::string CommandLine::helpText() const {
    std::ostringstream output;
    output << "Usage: " << programName_ << " [options]";
    if (positional_) {
        output << (positional_->required ? " <" : " [") << positional_->name;
        if (positional_->list) output << "...";
        output << (positional_->required ? '>' : ']');
    }
    output << "\n\n" << description_ << "\n\nOptions:\n";
    for (const auto& option : options_) {
        output << "  ";
        if (option.shortName != '\0') {
            output << '-' << option.shortName;
        } else {
            output << "  ";
        }
        if (!option.longName.empty()) {
            if (option.shortName != '\0') output << ", ";
            else output << "  ";
            output << "--" << option.longName;
        }
        if ((option.valueType == CommandLineValueType::String ||
             option.valueType == CommandLineValueType::Number) &&
            option.action == CommandLineAction::None) {
            output << (option.list ? " <value>..." : " <value>");
        }
        output << "\t" << option.description;
        if (option.valueType == CommandLineValueType::Counter) output << " (repeatable)";
        if (option.defaultValue) output << " (default: " << *option.defaultValue << ')';
        if (option.required) output << " (required)";
        output << '\n';
    }
    if (positional_) {
        output << "  " << positional_->name << "\t" << positional_->description;
        if (positional_->required) output << " (required)";
        output << '\n';
    }
    return output.str();
}

std::string CommandLine::versionText() const {
    return programName_ + ": v" + version_ + '\n';
}

} // namespace cwhip
