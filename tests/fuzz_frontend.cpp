#include "cwhip/Diagnostic.hpp"
#include "cwhip/Lexer.hpp"
#include "cwhip/Parser.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
    try {
        cwhip::Lexer lexer(std::string(reinterpret_cast<const char*>(data), size),
                          "<fuzz>");
        auto tokens = lexer.tokenize();
        cwhip::Parser parser(std::move(tokens));
        auto program = parser.parseProgram(false);
        (void)program;
    } catch (const cwhip::DiagnosticError&) {
        // Invalid source must be diagnosed; other exceptions and faults are bugs.
    }
    return 0;
}
