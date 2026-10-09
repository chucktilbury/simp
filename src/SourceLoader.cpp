#include "cwhip/SourceLoader.hpp"

#include "cwhip/Diagnostic.hpp"
#include "cwhip/Lexer.hpp"

#include <fstream>
#include <iterator>
#include <sstream>
#include <system_error>
#include <utility>

namespace cwhip {

std::vector<Token> tokenizeWithIncludes(
    const std::string& source, const std::filesystem::path& sourcePath,
    std::unordered_set<std::string>& includedFiles, std::size_t depth, bool root,
    std::size_t maximumIncludeDepth,
    std::vector<std::filesystem::path> includeChain,
    std::vector<std::filesystem::path> includeSearchPaths) {
    if (includeChain.empty()) includeChain.push_back(sourcePath);
    Lexer lexer(source, sourcePath.string());
    const auto tokens = lexer.tokenize();
    std::vector<Token> expanded;
    std::size_t braceDepth = 0;
    bool sawStart = false;
    for (std::size_t index = 0; index + 1 < tokens.size(); ++index) {
        const auto& token = tokens[index];
        if (!root && token.type == TokenType::Start) {
            throw DiagnosticError(token.location,
                                  "included source cannot declare 'start'");
        }
        if (token.type == TokenType::Include && braceDepth == 0) {
            if (sawStart) {
                throw DiagnosticError(token.location,
                                      "'include' must appear before top-level 'start'");
            }
            if (index + 1 >= tokens.size() - 1 ||
                tokens[index + 1].type != TokenType::String ||
                !tokens[index + 1].formattedString) {
                throw DiagnosticError(token.location,
                                      "expected a double-quoted path after 'include'");
            }
            const auto& pathToken = tokens[++index];
            const auto next = index + 1;
            if (tokens[next].type != TokenType::Newline &&
                tokens[next].type != TokenType::End) {
                throw DiagnosticError(tokens[next].location,
                                      "expected newline after include path");
            }

            const std::filesystem::path requested(pathToken.text);
            std::vector<std::filesystem::path> candidates;
            if (requested.is_absolute()) {
                candidates.push_back(requested);
            } else {
                candidates.push_back(sourcePath.parent_path() / requested);
                for (const auto& directory : includeSearchPaths) {
                    candidates.push_back(directory / requested);
                }
            }
            std::filesystem::path canonicalPath;
            for (const auto& candidate : candidates) {
                std::error_code error;
                canonicalPath = std::filesystem::canonical(candidate, error);
                if (!error) break;
                canonicalPath.clear();
            }
            if (canonicalPath.empty()) {
                throw DiagnosticError(pathToken.location,
                                      "cannot resolve included source '" +
                                          pathToken.text + "'");
            }
            const auto canonicalName = canonicalPath.string();
            if (includedFiles.emplace(canonicalName).second) {
                if (depth >= maximumIncludeDepth) {
                    auto chain = includeChain;
                    chain.push_back(canonicalPath);
                    std::ostringstream chainText;
                    for (std::size_t chainIndex = 0; chainIndex < chain.size(); ++chainIndex) {
                        if (chainIndex != 0) chainText << " -> ";
                        chainText << chain[chainIndex].string();
                    }
                    throw DiagnosticError(token.location,
                                          "maximum include depth of " +
                                              std::to_string(maximumIncludeDepth) +
                                              " exceeded (include chain: " +
                                              chainText.str() + ")");
                }
                std::ifstream included(canonicalPath);
                if (!included) {
                    throw DiagnosticError(pathToken.location,
                                          "cannot open included source '" +
                                              canonicalName + "'");
                }
                const std::string includedSource{
                    std::istreambuf_iterator<char>(included),
                    std::istreambuf_iterator<char>()};
                auto childChain = includeChain;
                childChain.push_back(canonicalPath);
                auto includedTokens = tokenizeWithIncludes(
                    includedSource, canonicalPath, includedFiles, depth + 1, false,
                    maximumIncludeDepth, std::move(childChain), includeSearchPaths);
                expanded.insert(expanded.end(),
                                std::make_move_iterator(includedTokens.begin()),
                                std::make_move_iterator(includedTokens.end()));
            }
            if (next < tokens.size() - 1) ++index;
            if (expanded.empty() || expanded.back().type != TokenType::Newline) {
                expanded.push_back({TokenType::Newline, "\n", token.location});
            }
            continue;
        }
        if (token.type == TokenType::Start && braceDepth == 0) sawStart = true;
        if (token.type == TokenType::LeftBrace) ++braceDepth;
        if (token.type == TokenType::RightBrace && braceDepth > 0) --braceDepth;
        expanded.push_back(token);
    }
    return expanded;
}

} // namespace cwhip
