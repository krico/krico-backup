#pragma once

#include <string>
#include <filesystem>
#include <memory>

namespace krico::backup {
    class TemporaryFile {
    public:
        static constexpr auto DEFAULT_PREFIX = "tmpf";
        static constexpr auto DEFAULT_SUFFIX = "";

        using ptr = std::unique_ptr<TemporaryFile>;

        struct args_t {
            std::filesystem::path dir{std::filesystem::temp_directory_path()};
            std::string prefix{DEFAULT_PREFIX};
            std::string suffix{DEFAULT_SUFFIX};
        };

        explicit TemporaryFile(const std::filesystem::path &dir,
                               const std::string &prefix = DEFAULT_PREFIX,
                               const std::string &suffix = DEFAULT_SUFFIX);

        explicit TemporaryFile(const args_t &args);

        explicit TemporaryFile();

        TemporaryFile(const TemporaryFile &) = delete;

        void operator=(const TemporaryFile &) = delete;

        ~TemporaryFile();

        [[nodiscard]] const std::filesystem::path &file() const;

    private:
        const std::filesystem::path file_;
    };
}
