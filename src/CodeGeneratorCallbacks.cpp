#include "simp/CodeGenerator.hpp"
#include "simp/CallbackType.hpp"

namespace simp {
namespace {

std::string cType(const std::string& type) {
    if (type == "int") return "int64_t";
    if (type == "unsigned") return "uint64_t";
    if (type == "float") return "double";
    if (type == "bool") return "_Bool";
    if (type == "void") return "void";
    return "void *";
}

std::string argumentMember(const std::string& type) {
    if (type == "int") return "integer";
    if (type == "unsigned") return "unsigned_integer";
    if (type == "float") return "floating";
    if (type == "bool") return "boolean";
    return "pointer";
}

} // namespace

std::string CodeGenerator::emitCallbackBridge(const std::string& type) {
    const auto symbol = callbackSymbol(type);
    const auto create = symbol + "_create";
    if (!declaredInlineSymbols_.emplace(create).second) return create;
    const auto signature = callbackSignature(type);
    std::string parameters = "SimpCallbackContext *context";
    std::string methodParameters = "void *";
    std::string callArguments = "simp_callback_receiver(callback)";
    std::string initialize;
    std::string managed;
    for (std::size_t index = 0; index < signature.parameters.size(); ++index) {
        const auto& parameter = signature.parameters[index];
        const auto number = std::to_string(index);
        parameters += ", " + cType(parameter) + " p" + number;
        methodParameters += ", " + cType(parameter);
        callArguments += ", arguments[" + number + "]." + argumentMember(parameter);
        initialize += "    arguments[" + number + "]." + argumentMember(parameter) +
                      " = p" + number + ";\n";
        if (index) managed += ", ";
        managed += isManagedReferenceType(parameter) ? "1" : "0";
    }
    const auto count = signature.parameters.size();
    std::string source = "static void " + symbol +
        "_invoke(void *callback, const SimpCallbackArgument *arguments, SimpCallbackArgument *result) {\n";
    if (!count) source += "    (void)arguments;\n";
    if (signature.result == "void") source += "    (void)result;\n    ";
    else source += "    result->" + argumentMember(signature.result) + " = ";
    source += "((" + cType(signature.result) + " (*)(" + methodParameters +
              "))simp_callback_code(callback))(" + callArguments + ");\n}\n";
    source += "static " + cType(signature.result) + " " + symbol + "_adapter(" + parameters +
              ") {\n    SimpCallbackArgument arguments[" +
              std::to_string(count ? count : 1) + "] = {{0}};\n"
              "    SimpCallbackArgument result = {0};\n" + initialize +
              "    simp_callback_context_invoke(context, \"" + type +
              "\", arguments, &result);\n";
    if (signature.result != "void")
        source += "    return result." + argumentMember(signature.result) + ";\n";
    source += "}\n__attribute__((weak)) void *" + create +
              "(void *receiver, void *code) {\n"
              "    static const uint8_t managed[] = {" + (managed.empty() ? "0" : managed) + "};\n"
              "    return simp_callback_new(receiver, code, (SimpCallbackAdapter)" +
              symbol + "_adapter, " + symbol + "_invoke, \"" + type + "\", " +
              std::to_string(count) + ", managed);\n}\n";
    inlineDeclarations_ += "declare ptr @" + create + "(ptr, ptr)\n";
    inlineShims_.emplace(create, std::move(source));
    return create;
}

CodeGenerator::Value CodeGenerator::emitBoundMethod(const Expression& expression) {
    const Expression* root = nullptr;
    const ClassDeclaration* owner = nullptr;
    std::vector<std::string> path;
    const bool qualified = resolveBaseQualifier(*expression.left, root, owner, path);
    auto receiver = emitExpression(qualified ? *root : *expression.left);
    const auto* complete = classes_.at(receiver.type);
    emitNullCheck(receiver.operand, expression.location);
    if (qualified)
        receiver = {owner->name, emitSubobjectAddress(receiver.operand, *complete, path)};
    else owner = complete;
    const auto code = emitMethodCode(receiver, *owner, expression.value, expression.resolvedSignature);
    const auto create = emitCallbackBridge(expression.resolvedType);
    const auto result = newTemporary();
    instructions_ += "  " + result + " = call ptr @" + create + "(ptr " + receiver.operand +
                     ", ptr " + code + ")\n";
    return rootObjectValue({expression.resolvedType, result}, expression.location);
}

CodeGenerator::Value CodeGenerator::emitCallbackCall(const Expression& expression) {
    const auto callback = emitExpression(*expression.left);
    const auto signature = callbackSignature(expression.resolvedType);
    const auto receiver = newTemporary();
    const auto code = newTemporary();
    instructions_ += "  " + receiver + " = call ptr @simp_callback_receiver(ptr " + callback.operand + ")\n";
    std::string arguments = "ptr " + receiver;
    for (std::size_t index = 0; index < signature.parameters.size(); ++index) {
        auto argument = emitExpression(*expression.arguments[index], signature.parameters[index]);
        argument = convertObjectValue(argument, signature.parameters[index],
                                      expression.arguments[index]->location);
        arguments += ", " + llvmType(signature.parameters[index]) + " " + argument.operand;
    }
    instructions_ += "  " + code + " = call ptr @simp_callback_code(ptr " + callback.operand + ")\n";
    const auto result = signature.result == "void" ? "" : newTemporary();
    instructions_ += "  " + (result.empty() ? "" : result + " = ") + "call " +
                     llvmType(signature.result) + " " + code + "(" + arguments + ")\n";
    return rootObjectValue({signature.result, result}, expression.location);
}

} // namespace simp
