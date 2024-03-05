#pragma once

#include <filesystem>

namespace krico::backup {
    struct settings final {
        settings() = default;

        explicit settings(const std::filesystem::path &file);
    };
}
