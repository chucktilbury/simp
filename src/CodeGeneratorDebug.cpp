#include "simp/CodeGenerator.hpp"

#include <filesystem>
#include <sstream>

namespace simp {

std::string CodeGenerator::debugQuote(const std::string& value) {
    std::string result = "\"";
    constexpr char digits[] = "0123456789ABCDEF";
    for (unsigned char c : value) {
        if (c == '"' || c == '\\' || c < 32 || c >= 127) {
            result += '\\';
            result += digits[c >> 4];
            result += digits[c & 15];
        } else {
            result += static_cast<char>(c);
        }
    }
    return result + "\"";
}

std::string CodeGenerator::debugNode(const std::string& contents) {
    const auto id = "!" + std::to_string(nextDebugNode_++);
    debugNodes_ += id + " = " + contents + "\n";
    return id;
}

std::string CodeGenerator::debugFile(const std::string& path) {
    const auto absolute = std::filesystem::absolute(
        path.empty() ? std::filesystem::path("simple") : std::filesystem::path(path)
    ).lexically_normal();
    const auto key = absolute.string();
    const auto found = debugFiles_.find(key);
    if (found != debugFiles_.end()) return found->second;
    const auto id = debugNode("!DIFile(filename: " + debugQuote(absolute.filename().string()) +
                              ", directory: " + debugQuote(absolute.parent_path().string()) +
                              ")");
    debugFiles_.emplace(key, id);
    return id;
}

std::string CodeGenerator::debugType(const std::string& type) {
    if (type == "void") return "null";
    const auto found = debugTypes_.find(type);
    if (found != debugTypes_.end()) return found->second;
    std::string id;
    if (type == "int" || type == "unsigned" || type == "float" || type == "bool") {
        const auto size = type == "int" ? 32 : type == "bool" ? 8 : 64;
        const auto encoding = type == "float" ? "DW_ATE_float" :
                              type == "bool" ? "DW_ATE_boolean" :
                              type == "unsigned" ? "DW_ATE_unsigned" : "DW_ATE_signed";
        id = debugNode("!DIBasicType(name: " + debugQuote(type) + ", size: " +
                       std::to_string(size) + ", encoding: " + encoding + ")");
    } else if (type == "any" || type == "type") {
        id = debugNode("!DICompositeType(tag: DW_TAG_structure_type, name: " +
                       debugQuote(type) + ", size: " +
                       (type == "any" ? "256" : "128") + ")");
    } else {
        const auto name = type == "string" ? "String" : type;
        const auto structure = debugNode("!DICompositeType(tag: DW_TAG_structure_type, name: " +
                                          debugQuote(name) + ", flags: DIFlagFwdDecl)");
        id = debugNode("!DIDerivedType(tag: DW_TAG_pointer_type, baseType: " + structure +
                       ", size: 64)");
    }
    debugTypes_.emplace(type, id);
    return id;
}

std::string CodeGenerator::debugBeginFunction(const SourceLocation& location,
                                               const std::string& name,
                                               const std::string& symbol,
                                               const std::string& returnType) {
    if (!debug_) return {};
    const auto file = debugFile(location.file);
    const auto signature = debugNode("!DISubroutineType(types: !{" +
                                      debugType(returnType) + "})");
    debugSubprogram_ = debugNode("distinct !DISubprogram(name: " + debugQuote(name) +
        ", linkageName: " + debugQuote(symbol.front() == '@' ? symbol.substr(1) : symbol) +
        ", scope: " + file + ", file: " + file +
        ", line: " + std::to_string(location.line) + ", type: " + signature +
        ", scopeLine: " + std::to_string(location.line) +
        ", spFlags: DISPFlagDefinition, unit: " + debugUnit_ + ")");
    debugScopes_.clear();
    debugLocations_.clear();
    debugCurrentLocation_ = debugLocation(location);
    return " !dbg " + debugSubprogram_;
}

std::string CodeGenerator::debugLocation(const SourceLocation& location) {
    if (!debug_) return {};
    const auto file = debugFile(location.file);
    auto scope = debugSubprogram_;
    const auto subprogramFile = debugScopes_.find("");
    if (subprogramFile == debugScopes_.end()) {
        debugScopes_[""] = file;
    } else if (file != subprogramFile->second) {
        const auto found = debugScopes_.find(file);
        if (found == debugScopes_.end()) {
            scope = debugNode("!DILexicalBlockFile(scope: " + debugSubprogram_ +
                              ", file: " + file + ", discriminator: 0)");
            debugScopes_.emplace(file, scope);
        } else {
            scope = found->second;
        }
    }
    const auto key = scope + ":" + std::to_string(location.line) + ":" +
                     std::to_string(location.column);
    const auto found = debugLocations_.find(key);
    if (found != debugLocations_.end()) return found->second;
    const auto id = debugNode("!DILocation(line: " + std::to_string(location.line) +
                              ", column: " + std::to_string(location.column) +
                              ", scope: " + scope + ")");
    debugLocations_.emplace(key, id);
    return id;
}

std::string CodeGenerator::debugDeclaration(const std::string& name,
                                            const std::string& type,
                                            const std::string& pointer,
                                            const SourceLocation& location,
                                            unsigned argument) {
    if (!debug_) return {};
    const auto file = debugFile(location.file);
    const auto variable = debugNode("!DILocalVariable(name: " + debugQuote(name) +
        ", arg: " + std::to_string(argument) + ", scope: " + debugSubprogram_ +
        ", file: " + file + ", line: " + std::to_string(location.line) +
        ", type: " + debugType(type) + ")");
    return "  call void @llvm.dbg.declare(metadata ptr " + pointer +
           ", metadata " + variable + ", metadata !DIExpression())\n";
}

std::string CodeGenerator::debugAnnotate(const std::string& body,
                                         const SourceLocation& fallback) {
    if (!debug_) return body;
    std::istringstream input(body);
    std::string result;
    std::string line;
    auto location = debugLocation(fallback);
    while (std::getline(input, line)) {
        constexpr char marker[] = "; simp.debug.location ";
        if (line.compare(0, sizeof(marker) - 1, marker) == 0) {
            location = line.substr(sizeof(marker) - 1);
            continue;
        }
        if (line.compare(0, 2, "  ") == 0 && !line.empty() &&
            line.find_first_not_of(' ') != std::string::npos) {
            line += ", !dbg " + location;
        }
        result += line + "\n";
    }
    return result;
}

void CodeGenerator::debugMetadata(const SourceLocation& source) {
    if (!debug_) return;
    const auto file = debugFile(source.file);
    debugUnit_ = debugNode("distinct !DICompileUnit(language: DW_LANG_C, file: " +
                          file + ", producer: \"simp\", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug)");
}

} // namespace simp
