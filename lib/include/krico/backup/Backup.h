#pragma once

#include "Directory.h"
#include "Digest.h"
#include <cstdint>
#include <filesystem>
#include <chrono>
#include <ostream>
#include <iomanip>

namespace krico::backup {
    class Backup {
    public:
        //!
        //! Create a new backup copying files from `source` into `target` as of `date`
        //!
        Backup(const std::filesystem::path &source, const std::filesystem::path &target,
               const std::chrono::year_month_day &date = {});

        [[nodiscard]] const std::filesystem::path &backup_dir() const { return backupDir_; }

        void run();

        struct statistics {
            uint64_t directories;
            uint64_t files;
            uint64_t files_copied;
            std::chrono::system_clock::time_point start_time;
            std::chrono::system_clock::time_point end_time;
        };

        [[nodiscard]] const statistics &stats() const { return statistics_; }

    private:
        Digest digest_;
        Directory source_;
        std::filesystem::path target_;
        std::chrono::year_month_day date_;
        std::filesystem::path backupDir_;
        statistics statistics_{};
        uint32_t directoryDepth_;

        void backup(const Directory &dir);

        void backup(const File &file);

        [[nodiscard]] std::filesystem::path digest(const File &file) const;
    };

    inline std::ostream &operator<<(std::ostream &out, const Backup::statistics &stats) {
        const auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(stats.end_time - stats.start_time);
        return out
               << "Directories  : " << std::setw(26) << stats.directories << std::endl
               << "Files        : " << std::setw(26) << stats.files << std::endl
               << "Copied files : " << std::setw(26) << stats.files_copied << std::endl
               << "Linked files : " << std::setw(26) << (stats.files - stats.files_copied) << std::endl
               << "Start time   : " << std::setw(26) << stats.start_time << std::endl
               << "End time     : " << std::setw(26) << stats.end_time << std::endl
               << "Elapsed      : " << std::setw(26) << std::format("{0:%T}", elapsed) << std::endl;
    }
}
