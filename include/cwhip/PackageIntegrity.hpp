#pragma once

#include <filesystem>
#include <string>

namespace cwhip {

std::string packageFileSha256(const std::filesystem::path& path);
std::string packageTreeSha256(const std::filesystem::path& root);

} // namespace cwhip
