#pragma once

#include "FileLock.h"
#include "BackupConfig.h"
#include "BackupDirectory.h"
#include "BackupRepositoryLog.h"
#include <filesystem>
#include <vector>

namespace krico::backup {
    //!
    //! Manages the interactions with meta-data of a backup target directory
    //!
    class BackupRepository final {
    public:
        static constexpr auto METADATA_DIR = ".krico-backup";
        static constexpr auto LOCK_FILE = "krico-backup.lock";
        static constexpr auto CONFIG_FILE = "config";
        static constexpr auto METADATA_SECTION = "metadata";
        static constexpr auto LOG_DIR = "log";
        static constexpr auto DIRECTORIES_DIR = "dirs";
        static constexpr auto HARDLINKS_DIR = "hlinks";

        explicit BackupRepository(const std::filesystem::path &dir);

        static BackupRepository initialize(const std::filesystem::path &dir);

        //!
        //! User visible directory of this repository
        //!
        [[nodiscard]] const std::filesystem::path &dir() const { return dir_; }

        //!
        //! Metadata directory of this repository
        //!
        [[nodiscard]] const std::filesystem::path &metaDir() const { return metaDir_; }

        //!
        //! Directory where log entries are stored
        //!
        [[nodiscard]] const std::filesystem::path &logDir() const { return logDir_; }

        //!
        //! Directory where BackupDirectory meta-data are stored
        //!
        [[nodiscard]] const std::filesystem::path &directoriesDir() const { return directoriesDir_; }

        //!
        //! Directory where BackupDirectory hard-links are stored
        //!
        [[nodiscard]] const std::filesystem::path &hardLinksDir() const { return hardLinksDir_; }

        //!
        //! Get this repository's config
        //!
        //! @throws krico::backup::exception if this repository is not locked
        //!
        [[nodiscard]] BackupConfig &config();

        //!
        //! Release the lock of this repository, this repository should no longer be used once unlocked
        //!
        void unlock();

        //!
        //! Adds `directory` to this repository to be a backup of `sourceDirectory`
        //!
        const BackupDirectory &add_directory(const std::filesystem::path &directory,
                                             const std::filesystem::path &sourceDirectory);

        std::vector<const BackupDirectory *> list_directories();

        BackupSummary run_backup(const BackupDirectory &directory);

        [[nodiscard]] BackupRepositoryLog &repositoryLog();

    private:
        const std::filesystem::path dir_;
        const std::filesystem::path metaDir_;
        FileLock lock_;
        BackupConfig config_;
        const std::filesystem::path logDir_;
        const std::filesystem::path directoriesDir_;
        const std::filesystem::path hardLinksDir_;
        bool directoriesLoaded_{false};
        std::vector<std::unique_ptr<BackupDirectory> > directories_{};
        std::unique_ptr<BackupRepositoryLog> repositoryLog_{nullptr};

        std::vector<std::unique_ptr<BackupDirectory> > &loadDirectories();
    };
}
