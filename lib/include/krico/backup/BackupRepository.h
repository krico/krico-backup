#pragma once

#include "FileLock.h"
#include "BackupConfig.h"
#include <filesystem>

namespace krico::backup {
    //!
    //! Manages the interactions with meta-data of a backup target directory
    //!
    class BackupRepository final {
    public:
        static constexpr auto METADATA = ".krico-backup";
        static constexpr auto LOCK_FILE = "krico-backup.lock";
        static constexpr auto CONFIG_FILE = "config";

        explicit BackupRepository(const std::filesystem::path &dir);

        static BackupRepository initialize(const std::filesystem::path &dir);

        [[nodiscard]] const std::filesystem::path &dir() const { return dir_; }

        //!
        //! Release the lock of this repository, this repository should no longer be used once unlocked
        //!
        void unlock();
    private:
        const std::filesystem::path dir_;
        const std::filesystem::path metaDir_;
        FileLock lock_;
        BackupConfig config_;
    };
}
