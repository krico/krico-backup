#pragma once

#include "BackupDirectory.h"
#include "Digest.h"
#include "Directory.h"
#include <chrono>

namespace krico::backup {
    //!
    //! Run a Backup for a given BackupDirectory.
    //!
    //! Not thread-safe, should be proteced by a BackupRepository lock
    //!
    class BackupRunner {
    public:
        static constexpr auto PREVIOUS_LINK = "previous";
        static constexpr auto CURRENT_LINK = "current";

        explicit BackupRunner(const BackupDirectory &directory);

        void run(); // TODO: return Backup

    private:
        const BackupDirectory &directory_;
        const std::chrono::year_month_day date_;
        const std::filesystem::path backupDir_;
        Digest digest_;

        [[nodiscard]] static std::filesystem::path determineBackupDir(const BackupDirectory &directory,
                                                                      const std::chrono::year_month_day &date);

        void backup(const Directory &dir);

        void backup(const File &file);

        void backup(const Symlink &symlink);

        [[nodiscard]] std::filesystem::path digest(const File &file) const;

        void adjust_symlinks() const;
    };
}
