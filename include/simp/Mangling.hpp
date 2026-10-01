/**
 * @file Mangling.hpp
 * @brief Shared parameter-type encoding for overload-aware symbol mangling.
 *
 * Simple supports method overloading, so a method's symbol name and its
 * virtual-dispatch slot must both be keyed by the full parameter-type list,
 * not just the method name. Both the semantic analyzer and the code
 * generator need exactly the same encoding, so it lives here.
 *
 * The encoding is a compact, C++-like type code per parameter. Codes are
 * concatenated and joined to the method name with '$', which cannot appear
 * in a Simple identifier or namespace path, so a mangled symbol can never
 * collide with an unmangled one.
 */
#ifndef SIMP_MANGLING_HPP
#define SIMP_MANGLING_HPP

#include "simp/Ast.hpp"

#include <string>
#include <vector>

namespace simp {

/// Encodes one parameter type as a short, unambiguous code.
inline std::string mangleTypeCode(const std::string& type) {
    if (type == "int") return "i";
    if (type == "unsigned") return "u";
    if (type == "float") return "f";
    if (type == "bool") return "b";
    if (type == "any") return "y";
    if (type == "void") return "v";
    if (type == "list") return "a";
    if (type == "dict") return "m";
    if (type == "buffer") return "B";
    if (type == "handle") return "h";
    // A class type, possibly namespace-qualified. Length-prefixing keeps the
    // dotted path unambiguous when codes are concatenated.
    return "C" + std::to_string(type.size()) + type;
}

/// Encodes a parameter list; empty for a method that takes no parameters.
inline std::string mangleParameters(const std::vector<Parameter>& parameters) {
    if (parameters.empty()) return {};
    std::string encoded = "$";
    for (const auto& parameter : parameters) encoded += mangleTypeCode(parameter.type);
    return encoded;
}

/// Encodes an already-resolved list of argument types, for call sites.
inline std::string mangleArgumentTypes(const std::vector<std::string>& types) {
    if (types.empty()) return {};
    std::string encoded = "$";
    for (const auto& type : types) encoded += mangleTypeCode(type);
    return encoded;
}

/// The overload-distinguishing key for a method: its name plus parameters.
inline std::string methodSignatureKey(const MethodDeclaration& method) {
    return method.name + mangleParameters(method.parameters);
}

/// A human-readable signature for diagnostics, e.g. "absolute(int)".
inline std::string readableSignature(const MethodDeclaration& method) {
    std::string text = method.name + "(";
    for (std::size_t index = 0; index < method.parameters.size(); ++index) {
        if (index != 0) text += ", ";
        text += method.parameters[index].type;
    }
    return text + ")";
}

/// True when two declarations have identical parameter type lists.
inline bool sameParameterTypes(const MethodDeclaration& left,
                               const MethodDeclaration& right) {
    if (left.parameters.size() != right.parameters.size()) return false;
    for (std::size_t index = 0; index < left.parameters.size(); ++index) {
        if (left.parameters[index].type != right.parameters[index].type) return false;
    }
    return true;
}

} // namespace simp

#endif
