#pragma once

#include "simp/Ast.hpp"
#include <string>
#include <vector>

namespace simp {

inline bool isCallbackType(const std::string& type) {
    return type.compare(0, 9, "callback<") == 0;
}

struct CallbackSignature {
    std::string result;
    std::vector<std::string> parameters;
};

inline CallbackSignature callbackSignature(const std::string& type) {
    CallbackSignature signature;
    std::size_t depth = 0;
    std::size_t open = 9;
    for (; open < type.size(); ++open) {
        if (type[open] == '<') ++depth;
        else if (type[open] == '>') --depth;
        else if (type[open] == '(' && depth == 0) break;
    }
    signature.result = type.substr(9, open - 9);
    std::size_t first = open + 1;
    depth = 0;
    for (std::size_t index = first; index + 2 < type.size(); ++index) {
        if (type[index] == '<') ++depth;
        else if (type[index] == '>') --depth;
        else if (type[index] == ',' && depth == 0) {
            signature.parameters.push_back(type.substr(first, index - first));
            first = index + 1;
        }
    }
    if (first < type.size() - 2)
        signature.parameters.push_back(type.substr(first, type.size() - 2 - first));
    return signature;
}

inline std::string callbackType(const std::string& result,
                               const std::vector<std::string>& parameters) {
    std::string type = "callback<" + result + "(";
    for (std::size_t index = 0; index < parameters.size(); ++index) {
        if (index) type += ",";
        type += parameters[index];
    }
    return type + ")>";
}

inline std::string callbackType(const MethodDeclaration& method) {
    std::vector<std::string> parameters;
    for (const auto& parameter : method.parameters) parameters.push_back(parameter.type);
    return callbackType(method.returnType, parameters);
}

inline std::string callbackSymbol(const std::string& type) {
    static const char hex[] = "0123456789abcdef";
    std::string result = "simp_callback_";
    for (unsigned char byte : type) {
        result += hex[byte >> 4];
        result += hex[byte & 15];
    }
    return result;
}

} // namespace simp
