#pragma once

#include <filesystem>
#include <string>

namespace simp {

std::string packageFileSha256(const std::filesystem::path& path);
std::string packageTreeSha256(const std::filesystem::path& root);

} // namespace simp
