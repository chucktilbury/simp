#include "simp/PackageIntegrity.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace simp {
namespace {

class Sha256 {
public:
    void update(const char* data, std::size_t size) {
        bytes_ += size;
        for (std::size_t index = 0; index < size; ++index) {
            block_[used_++] = static_cast<unsigned char>(data[index]);
            if (used_ == block_.size()) {
                compress();
                used_ = 0;
            }
        }
    }

    std::string finish() {
        const auto bits = bytes_ * 8;
        block_[used_++] = 0x80;
        if (used_ > 56) {
            std::fill(block_.begin() + used_, block_.end(), 0);
            compress();
            used_ = 0;
        }
        std::fill(block_.begin() + used_, block_.begin() + 56, 0);
        for (unsigned index = 0; index < 8; ++index) {
            block_[63 - index] = static_cast<unsigned char>(bits >> (index * 8));
        }
        compress();
        std::ostringstream output;
        output << std::hex << std::setfill('0');
        for (const auto word : state_) output << std::setw(8) << word;
        return output.str();
    }

private:
    static std::uint32_t rotate(std::uint32_t value, unsigned count) {
        return (value >> count) | (value << (32 - count));
    }

    void compress() {
        static constexpr std::array<std::uint32_t, 64> constants{
            0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1,
            0x923f82a4, 0xab1c5ed5, 0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
            0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174, 0xe49b69c1, 0xefbe4786,
            0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
            0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147,
            0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
            0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85, 0xa2bfe8a1, 0xa81a664b,
            0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
            0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a,
            0x5b9cca4f, 0x682e6ff3, 0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
            0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};
        std::array<std::uint32_t, 64> words{};
        for (std::size_t index = 0; index < 16; ++index) {
            for (unsigned byte = 0; byte < 4; ++byte) {
                words[index] = (words[index] << 8) | block_[index * 4 + byte];
            }
        }
        for (std::size_t index = 16; index < words.size(); ++index) {
            const auto left = words[index - 15];
            const auto right = words[index - 2];
            words[index] = words[index - 16] +
                (rotate(left, 7) ^ rotate(left, 18) ^ (left >> 3)) +
                words[index - 7] +
                (rotate(right, 17) ^ rotate(right, 19) ^ (right >> 10));
        }
        auto work = state_;
        for (std::size_t index = 0; index < words.size(); ++index) {
            const auto first = work[7] +
                (rotate(work[4], 6) ^ rotate(work[4], 11) ^ rotate(work[4], 25)) +
                ((work[4] & work[5]) ^ (~work[4] & work[6])) +
                constants[index] + words[index];
            const auto second =
                (rotate(work[0], 2) ^ rotate(work[0], 13) ^ rotate(work[0], 22)) +
                ((work[0] & work[1]) ^ (work[0] & work[2]) ^ (work[1] & work[2]));
            for (unsigned position = 7; position > 0; --position) work[position] = work[position - 1];
            work[4] += first;
            work[0] = first + second;
        }
        for (std::size_t index = 0; index < state_.size(); ++index) state_[index] += work[index];
    }

    std::array<std::uint32_t, 8> state_{
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};
    std::array<unsigned char, 64> block_{};
    std::size_t used_ = 0;
    std::uint64_t bytes_ = 0;
};

void hashFile(Sha256& hash, const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) throw std::runtime_error("cannot read package integrity input: " + path.string());
    std::array<char, 8192> buffer{};
    while (input.read(buffer.data(), buffer.size()) || input.gcount() > 0) {
        hash.update(buffer.data(), static_cast<std::size_t>(input.gcount()));
    }
    if (input.bad()) throw std::runtime_error("cannot hash package file: " + path.string());
}

} // namespace

std::string packageFileSha256(const std::filesystem::path& path) {
    Sha256 hash;
    hashFile(hash, path);
    return hash.finish();
}

std::string packageTreeSha256(const std::filesystem::path& root) {
    if (std::filesystem::is_symlink(root) || !std::filesystem::is_directory(root)) {
        throw std::runtime_error("package integrity root must be a real directory: " + root.string());
    }
    std::vector<std::string> files;
    for (std::filesystem::recursive_directory_iterator it(root), end; it != end; ++it) {
        const auto relative = it->path().lexically_relative(root);
        if (*relative.begin() == ".git") {
            if (it->is_directory()) it.disable_recursion_pending();
            continue;
        }
        const auto status = it->symlink_status();
        if (std::filesystem::is_directory(status)) continue;
        if (!std::filesystem::is_regular_file(status)) {
            throw std::runtime_error("package integrity rejects symlinks and special files: " +
                                     it->path().string());
        }
        files.push_back(relative.generic_string());
    }
    std::sort(files.begin(), files.end());
    Sha256 hash;
    const char separator = '\0';
    for (const auto& file : files) {
        hash.update(file.data(), file.size());
        hash.update(&separator, 1);
        hashFile(hash, root / file);
        hash.update(&separator, 1);
    }
    return hash.finish();
}

} // namespace simp
