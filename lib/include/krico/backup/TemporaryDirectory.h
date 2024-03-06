#pragma once

#include <string>
#include <filesystem>
#include <memory>

namespace krico::backup {
    class TemporaryDirectory {
    public:
        static constexpr auto DEFAULT_PREFIX = "tmpd";

        using ptr = std::unique_ptr<TemporaryDirectory>;

        struct args_t {
            std::filesystem::path dir{std::filesystem::temp_directory_path()};
            std::string prefix{DEFAULT_PREFIX};
        };

        explicit TemporaryDirectory(const std::filesystem::path &dir,
                                    const std::string &prefix = DEFAULT_PREFIX);

        explicit TemporaryDirectory(const args_t &args);

        explicit TemporaryDirectory();

        TemporaryDirectory(const TemporaryDirectory &) = delete;

        void operator=(const TemporaryDirectory &) = delete;

        ~TemporaryDirectory();

        [[nodiscard]] const std::filesystem::path &dir() const;

    private:
        const std::filesystem::path dir_;
    };
}
