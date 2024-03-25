#pragma once

#include "log_records.h"
#include "Digest.h"
#include "BackupSummary.h"
#include "gtest/gtest_prod.h"
#include <filesystem>
#include <ostream>
#include <istream>
#include <cassert>

namespace krico::backup {
    //!
    //! Manages the log_entry chain
    //!
    //! Not thread-safe, must be protected by BackupRepository lock
    //!
    class BackupRepositoryLog {
    public:
        static constexpr auto HEAD_FILE = "HEAD";
        static constexpr auto DIGEST_DIRS = 1;

        explicit BackupRepositoryLog(std::filesystem::path dir);

        [[nodiscard]] const Digest::result &head();

        //!
        //! Produces a list of Digest::result that start with `hash`
        //!
        [[nodiscard]] std::vector<Digest::result> findHash(const std::string &hash) const;

        //!
        //! Add an InitRecord to the BackupRepositoryLog.
        //!
        void putInitRecord(const std::string &author);

        //!
        //! Add an AddDirectoryRecord to the BackupRepositoryLog.
        //!
        void putAddDirectoryRecord(const std::string &author,
                                   const std::string &directoryId,
                                   const std::filesystem::path &sourceDir);

        //!
        //! Add an RunBackupRecord to the BackupRepositoryLog.
        //!
        void putRunBackupRecord(const std::string &author, const BackupSummary &summary);

        //!
        //! Retrieve a given LogHeader **only valid** until the next call to getRecord(), getHeadRecord() or getPrev()
        //!
        //! @return the record with the given Digest::result
        //! @throws krico::backup::exception if the entry is not found or cannot be read
        //!
        [[nodiscard]] const LogHeader &getRecord(const Digest::result &digest);

        //!
        //! @return getRecord(head())
        //!
        [[nodiscard]] const LogHeader &getHeadRecord() { return getRecord(head()); }

        //!
        //! @return getRecord(entry.prev())
        //!
        [[nodiscard]] const LogHeader &getPrev(const LogHeader &entry) { return getRecord(entry.prev()); }

    private:
        const std::filesystem::path dir_;
        const std::filesystem::path headFile_;
        Digest::result head_;
        Digest digest_;

        struct readers {
            LogHeader header_{};
            InitRecord init_{};
            AddDirectoryRecord add_{};
            RunBackupRecord run_{};
        } readers_{};

        void putRecord(LogHeader &entry);

        FRIEND_TEST(BackupRepositoryLogTest, putRecord);
    };
}
