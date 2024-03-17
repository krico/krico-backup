#pragma once

#include <string>
#include <filesystem>

namespace krico::backup {
    //!
    //! Unique identifier of a BackupDirectory
    //!
    struct BackupDirectoryId {
        explicit BackupDirectoryId(const std::string &id);

        //!
        //! string representation
        //!
        [[nodiscard]] const std::string &str() const {
            return id_;
        }

        //!
        //! Directory path relative to BackupRepository::dir
        //!
        [[nodiscard]] const std::filesystem::path &relative_path() const {
            return relativePath_;
        }

        //!
        //! Directory path relative to BackupRepository::metaDir
        //!
        [[nodiscard]] const std::filesystem::path &id_path() const {
            return idPath_;
        }

        friend bool operator==(const BackupDirectoryId &lhs, const BackupDirectoryId &rhs) {
            return lhs.idPath_ == rhs.idPath_;
        }

        friend bool operator!=(const BackupDirectoryId &lhs, const BackupDirectoryId &rhs) {
            return !(lhs == rhs);
        }

    private:
        std::string id_;
        std::filesystem::path relativePath_;
        std::filesystem::path idPath_;
    };
}
