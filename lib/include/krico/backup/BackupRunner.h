#pragma once

#include "BackupDirectory.h"
#include "BackupSummary.h"
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
        static constexpr auto DIGEST_DIRS = 2;

        explicit BackupRunner(const BackupDirectory &directory, const std::chrono::year_month_day &date = {});

        [[nodiscard]] BackupSummary run();

        [[nodiscard]] const std::filesystem::path &backupDir() const { return backupDir_; }

    private:
        const BackupDirectory &directory_;
        const std::chrono::year_month_day date_;
        const std::filesystem::path backupDir_;
        Digest digest_;

        [[nodiscard]] static std::filesystem::path determineBackupDir(const BackupDirectory &directory,
                                                                      const std::chrono::year_month_day &date);

        void backup(BackupSummaryBuilder &builder, const Directory &dir);

        void backup(BackupSummaryBuilder &builder, const File &file);

        void backup(BackupSummaryBuilder &builder, const Symlink &symlink);

        [[nodiscard]] Digest::result digest(const File &file) const;

        void adjustSymlinks(BackupSummaryBuilder &builder) const;
    };
}
