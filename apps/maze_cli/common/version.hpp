#ifndef VERSION_HPP
#define VERSION_HPP

#include <string_view>

#include "common/kernel_version.hpp"

namespace MazeCommon {

constexpr std::string_view kCliVersion = "0.3.0";
constexpr std::string_view kVersion = kCliVersion;  // backward compatibility

constexpr std::string_view kVersionDisplayPrefix = "cli=";
constexpr std::string_view kKernelDisplayPrefix = ", kernel=";

}  // namespace MazeCommon

#endif  // VERSION_HPP
