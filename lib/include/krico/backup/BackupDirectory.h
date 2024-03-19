#pragma once

#include "BackupDirectoryId.h"
#include "gtest/gtest_prod.h"

namespace krico::backup {
    class BackupRepository; // fwd-decl

    class BackupDirectory {
    public:
        using ptr = std::unique_ptr<BackupDirectory>;

        static constexpr auto TARGET_FILE = "target";
        static constexpr auto SOURCE_FILE = "source";

        //!
        //! Construct a non-configured backup directory
        //!
        BackupDirectory(BackupRepository &repository, BackupDirectoryId id);

        //!
        //! Construct a nonconfigured backup directory
        //!
        BackupDirectory(BackupRepository &repository, const std::filesystem::path &directoryMetaDir);

        //!
        //! @return the BackupRepository owning this backup directory
        //!
        [[nodiscard]] BackupRepository &repository() const { return repository_; }

        //!
        //! @return the BackupDirectoryId that uniquely identifies this backup directory
        //!
        [[nodiscard]] const BackupDirectoryId &id() const { return id_; }

        //!
        //! @return true if this directory is configured in the BackupRepository, false if it needs to be added
        //!
        [[nodiscard]] bool configured() const { return configured_; }

        //!
        //! User visible path (sub-path of BackupRepository::dir)
        //!
        [[nodiscard]] const std::filesystem::path &dir() const { return dir_; }

        //!
        //! Metadata directory path (sub-path of BackupRepository::metaDir)
        //!
        [[nodiscard]] const std::filesystem::path &metaDir() const { return metaDir_; }

        //!
        //! The sourceDir (directory being backed-up) for this BackupDirectory.
        //!
        //! @throws krico::backup::exception if this direcotry is not configured().
        //!
        [[nodiscard]] const std::filesystem::path &sourceDir() const;

        friend bool operator==(const BackupDirectory &lhs, const BackupDirectory &rhs) {
            return lhs.id_ == rhs.id_ && &lhs.repository_ == &rhs.repository_;
        }

        friend bool operator!=(const BackupDirectory &lhs, const BackupDirectory &rhs) {
            return !(lhs == rhs);
        }

    private:
        BackupRepository &repository_;
        const BackupDirectoryId id_;
        std::filesystem::path dir_;
        std::filesystem::path metaDir_;
        bool configured_;
        std::filesystem::path sourceDir_;

        //!
        //! Configure the sourceDir() of this BackupDirectory (essentially adding it to a BackupRepository).
        //!
        //! After a call to this method, configured() should return true.
        //!
        //! @throws krico::backup::exception if this directory is already configured()
        //!
        void configure(const std::filesystem::path &sourceDir);

        //!
        //! Build the BackupDirectoryId for a directory confirugred under directoryMetaDir
        //!
        static BackupDirectoryId readId(const std::filesystem::path &directoryMetaDir);

        friend class BackupRepository;
        FRIEND_TEST(BackupDirectoryTest, configure);
    };
}
