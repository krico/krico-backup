#pragma once

#include <filesystem>
#include <map>
#include <optional>
#include <cstdint>

namespace krico::backup {
    //!
    //! Manages the configuration of a BackupRepository based on
    //! [gitconfig's syntax](https://git-scm.com/docs/git-config#_syntax).
    //!
    //! Not thread-safe, expected to be protected by BackupRepository's lock.
    //!
    class BackupConfig final {
    public:
        explicit BackupConfig(std::filesystem::path file);

        [[nodiscard]] const std::filesystem::path &file() const { return file_; }

        std::optional<std::string> get(const std::string &key) {
            if (const auto found = list_.find(key); found != list_.end()) return found->second;
            return std::nullopt;
        }

        std::optional<std::string> get(const std::string &section, const std::string &variable) {
            return get(section, "", variable);
        }

        std::optional<std::string> get(const std::string &section,
                                       const std::string &subSection,
                                       const std::string &variable);

        void set(const std::string &section,
                 const std::string &subSection,
                 const std::string &variable,
                 const std::string &value);

        void set(const std::string &key, const std::string &value);

        //!
        //! Obtain a list of all options in the dot form (e.g. `{<section>.[<sub-section>.]<variable>, <value>}`)
        //!
        [[nodiscard]] const std::map<std::string, std::string> &list() const { return list_; }

    private:
        struct value {
            uint32_t lineNo_;
            std::string value_;

            value(const uint32_t lineNo, std::string value): lineNo_(lineNo), value_(std::move(value)) {
            }
        };

        struct sub_section {
            uint32_t lineNo_;
            std::string name_;
            std::map<std::string, value> values_{};

            sub_section(const uint32_t lineNo, std::string name): lineNo_(lineNo), name_(std::move(name)) {
            }
        };

        struct section {
            std::string name_;
            std::map<std::string, sub_section> subSections_{};

            explicit section(std::string name): name_{std::move(name)} {
            }
        };

        const std::filesystem::path file_;
        std::map<std::string, section> sections_{};
        std::vector<std::string> lines_{};
        std::map<std::string, std::string> list_{};

        void initialize() const;

        void parse();
    };
}
