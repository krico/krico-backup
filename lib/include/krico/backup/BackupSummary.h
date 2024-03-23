#pragma once

#include "Digest.h"
#include "BackupDirectoryId.h"
#include "TemporaryFile.h"
#include "gtest/gtest_prod.h"
#include <filesystem>
#include <chrono>
#include <ostream>
#include <istream>
#include <fstream>

namespace krico::backup {
    class BackupSummaryBuilder; // fwd-decl

    class BackupSummary {
    public:
        using ptr = std::unique_ptr<BackupSummary>;
        static constexpr auto SUMMARY_FILE_SUFFIX = ".summary";

        explicit BackupSummary(const BackupSummaryBuilder &builder);

        explicit BackupSummary(std::istream &in);

        [[nodiscard]] const BackupDirectoryId &directoryId() const { return directoryId_; }
        [[nodiscard]] const std::chrono::year_month_day &date() const { return date_; }
        [[nodiscard]] const std::filesystem::path &backupId() const { return backupId_; }
        [[nodiscard]] const std::chrono::system_clock::time_point &startTime() const { return startTime_; }
        [[nodiscard]] const std::chrono::system_clock::time_point &endTime() const { return endTime_; }
        [[nodiscard]] const uint32_t &numDirectories() const { return numDirectories_; }
        [[nodiscard]] const uint32_t &numCopiedFiles() const { return numCopiedFiles_; }
        [[nodiscard]] const uint32_t &numHardLinkedFiles() const { return numHardLinkedFiles_; }
        [[nodiscard]] const uint32_t &numSymlinks() const { return numSymlinks_; }
        [[nodiscard]] const std::filesystem::path &previousTarget() const { return previousTarget_; }
        [[nodiscard]] const std::filesystem::path &currentTarget() const { return currentTarget_; }
        [[nodiscard]] const Digest::result &checksum() const { return checksum_; }

        //!
        //! Reconstruct the summary file for this BackupSummary given a `directoryMetaDir`
        //!
        [[nodiscard]] std::filesystem::path summaryFile(const std::filesystem::path &directoryMetaDir) const;

        void write(std::ostream &out) const;

    private:
        BackupDirectoryId directoryId_;
        std::chrono::year_month_day date_;
        std::filesystem::path backupId_;
        std::chrono::system_clock::time_point startTime_;
        std::chrono::system_clock::time_point endTime_;
        uint32_t numDirectories_{0};
        uint32_t numCopiedFiles_{0};
        uint32_t numHardLinkedFiles_{0};
        uint32_t numSymlinks_{0};
        std::filesystem::path previousTarget_;
        std::filesystem::path currentTarget_;
        Digest::result checksum_{};

        friend std::ostream &operator<<(std::ostream &out, const BackupSummary &summary) {
            using namespace std::chrono;
            static constexpr auto WIDTH = 2 * DigestLength::SHA1;
            const auto elapsed = duration_cast<nanoseconds>(summary.endTime_ - summary.startTime_);
            return out
                   << "DirectoryId  : " << std::setw(WIDTH) << summary.directoryId_.str() << std::endl
                   << "Date         : " << std::setw(WIDTH) << summary.date_ << std::endl
                   << "BackupId     : " << std::setw(WIDTH) << summary.backupId_.string() << std::endl
                   << "Start time   : " << std::setw(WIDTH) << summary.startTime_ << std::endl
                   << "End time     : " << std::setw(WIDTH) << summary.endTime_ << std::endl
                   << "Directories  : " << std::setw(WIDTH) << summary.numDirectories_ << std::endl
                   << "Copied files : " << std::setw(WIDTH) << summary.numCopiedFiles_ << std::endl
                   << "Hardlinks    : " << std::setw(WIDTH) << summary.numHardLinkedFiles_ << std::endl
                   << "Symlinks     : " << std::setw(WIDTH) << summary.numSymlinks_ << std::endl
                   << "Unliked bkp  : " << std::setw(WIDTH) << summary.previousTarget_.string() << std::endl
                   << "Previous bkp : " << std::setw(WIDTH) << summary.currentTarget_.string() << std::endl
                   << "Checksum     : " << std::setw(WIDTH) << summary.checksum_.str() << std::endl
                   << "Elapsed      : " << std::setw(WIDTH) << std::format("{0:%T}", elapsed);
        }
    };

    class BackupSummaryBuilder {
    public:
        BackupSummaryBuilder(const std::filesystem::path &metaDir,
                             BackupDirectoryId directoryId,
                             const std::chrono::year_month_day &date,
                             std::filesystem::path backupId);

        void addDir(const std::filesystem::path &dir);

        void addCopiedFile(const std::filesystem::path &file, const Digest::result &digest);

        void addHardLinkedFile(const std::filesystem::path &file, const Digest::result &digest);

        void addSymlink(const std::filesystem::path &file, const std::filesystem::path &target);

        void addPreviousSymlink(const std::filesystem::path &previousTarget);

        void addCurrentSymlink(const std::filesystem::path &currentTarget);

        [[nodiscard]] BackupSummary build();

    private:
        BackupDirectoryId directoryId_;
        std::chrono::year_month_day date_;
        std::filesystem::path backupId_;
        std::filesystem::path summaryFile_;
        TemporaryFile tmpFile_;
        std::ofstream out_;
        Digest digest_;

        std::chrono::system_clock::time_point startTime_;
        std::chrono::system_clock::time_point endTime_{};
        uint32_t numDirectories_{0};
        uint32_t numCopiedFiles_{0};
        uint32_t numHardLinkedFiles_{0};
        uint32_t numSymlinks_{0};
        std::filesystem::path previousTarget_{};
        std::filesystem::path currentTarget_{};
        Digest::result checksum_{};

        friend class BackupSummary;
        FRIEND_TEST(BackupRepositoryLogTest, putRunBackupLogEntry);
    };
}
