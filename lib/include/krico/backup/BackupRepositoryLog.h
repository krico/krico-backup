#pragma once

#include "Digest.h"
#include "BackupSummary.h"
#include "gtest/gtest_prod.h"
#include <filesystem>
#include <ostream>
#include <istream>

namespace krico::backup {
    enum class LogEntryType : uint8_t {
        NONE = 0,
        Initialized = 1,
        AddDirectory = 2,
        RunBackup = 3,
    };

    namespace lengths {
        namespace LogEntry {
            static constexpr size_t Type = 1;
            static constexpr size_t Hash = DigestLength::SHA1;
            static constexpr size_t Ts = 8;
            static constexpr size_t TotalLength = Type + Hash + Ts;
        }

        namespace AddDirectoryLogEntry {
            static constexpr size_t AuthorLength = 1;
            static constexpr size_t DirectoryIdLength = 2;
            static constexpr size_t SourceDirLength = 2;
            //!
            //! [uint8_t, uint16_t, uint16_t] respective lengths of
            //!  - AddDirectoryLogEntry::author
            //!  - AddDirectoryLogEntry::directoryId
            //!  - AddDirectoryLogEntry::sourcePath
            //!
            static constexpr size_t Lengths = AuthorLength + DirectoryIdLength + SourceDirLength;
        }
    }

    namespace offsets {
        namespace LogEntry {
            static constexpr size_t Type = 0;
            static constexpr size_t Hash = Type + lengths::LogEntry::Type;
            static constexpr size_t Ts = Hash + lengths::LogEntry::Hash;
            static constexpr size_t EndOfAttributes = Ts + lengths::LogEntry::Ts;
            static_assert(EndOfAttributes == lengths::LogEntry::TotalLength);
        }

        namespace AddDirectoryLogEntry {
            static constexpr size_t Lengths = 0;
            static constexpr size_t AuthorLength = 0;
            static constexpr size_t DirectoryIdLength = AuthorLength + lengths::AddDirectoryLogEntry::AuthorLength;
            static constexpr size_t SourceDirLength = DirectoryIdLength
                                                      + lengths::AddDirectoryLogEntry::DirectoryIdLength;
            static constexpr size_t Author = Lengths + lengths::AddDirectoryLogEntry::Lengths;
        }
    }

    class LogEntry {
    public:
        using ptr = std::unique_ptr<LogEntry>;

        LogEntry();

        explicit LogEntry(LogEntryType t);

        virtual ~LogEntry() = default;

        [[nodiscard]] LogEntryType type() const;


        [[nodiscard]] Digest::result prev() const;


        [[nodiscard]] uint64_t ts() const;

        virtual void update(const Digest &digest) const;

        virtual void write(std::ostream &out) const;

        virtual void read(std::istream &in);

    private:
        uint8_t header_[lengths::LogEntry::TotalLength];

        void type(LogEntryType type);

        void prev(const Digest::result &prev);

        void ts(uint64_t ts);

        friend class BackupRepositoryLog;
        FRIEND_TEST(BackupRepositoryLogTest, LogEntry);
        FRIEND_TEST(BackupRepositoryLogTest, addLogEntry);
    };

    class InitLogEntry final : public LogEntry {
    public:
        InitLogEntry();

        explicit InitLogEntry(std::string author);

        [[nodiscard]] const std::string &author() const { return author_; }

        void update(const Digest &digest) const override;

        void write(std::ostream &out) const override;

        void read(std::istream &in) override;

    private:
        std::string author_;
    };

    class AddDirectoryLogEntry final : public LogEntry {
    public:
        AddDirectoryLogEntry();

        ~AddDirectoryLogEntry() override;

        AddDirectoryLogEntry(const std::string &author,
                             const std::string &directoryId,
                             const std::filesystem::path &sourceDir);

        [[nodiscard]] std::string_view author() const;

        [[nodiscard]] std::string_view directoryId() const;

        [[nodiscard]] std::string_view sourceDir() const;

        void update(const Digest &digest) const override;

        void write(std::ostream &out) const override;

        void read(std::istream &in) override;

    private:
        uint8_t *buffer_{nullptr};
        uint8_t authorLength_{0};
        uint16_t directoryIdLength_{0};
        uint16_t sourceDirLength_{0};

        [[nodiscard]] size_t bufferSize() const;
    };

    class RunBackupLogEntry final : public LogEntry {
    public:
        RunBackupLogEntry();

        explicit RunBackupLogEntry(const BackupSummary &summary);

        [[nodiscard]] const BackupSummary &summary() const;

        void update(const Digest &digest) const override;

        void write(std::ostream &out) const override;

        void read(std::istream &in) override;

    private:
        std::unique_ptr<BackupSummary> summary_{nullptr};
    };

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
        //! Add an InitLogEntry to the BackupRepositoryLog.
        //!
        void putInitLogEntry(const std::string &author);

        //!
        //! Add an AddDirectoryLogEntry to the BackupRepositoryLog.
        //!
        void putAddDirectoryLogEntry(const std::string &author,
                                     const std::string &directoryId,
                                     const std::filesystem::path &sourceDir);

        //!
        //! Add an RunBackupLogEntry to the BackupRepositoryLog.
        //!
        void putRunBackupLogEntry(const BackupSummary &summary);

        //!
        //! Retrieve a given LogEntry **only valid** until the next call to getLogEntry(), getHeadLogEntry() or getPrev()
        //!
        //! @return the LogEntry with the given Digest::result
        //! @throws krico::backup::exception if the entry is not found or cannot be read
        //!
        [[nodiscard]] const LogEntry &getLogEntry(const Digest::result &digest);

        //!
        //! @return getLogEntry(head())
        //!
        [[nodiscard]] const LogEntry &getHeadLogEntry() { return getLogEntry(head()); }

        //!
        //! @return getLogEntry(entry.prev())
        //!
        [[nodiscard]] const LogEntry &getPrev(const LogEntry &entry) { return getLogEntry(entry.prev()); }

    private:
        const std::filesystem::path dir_;
        const std::filesystem::path headFile_;
        Digest::result head_;
        Digest digest_;
        LogEntry::ptr readLogEntry_{nullptr};

        void putLogEntry(LogEntry &entry);

        FRIEND_TEST(BackupRepositoryLogTest, putLogEntry);
    };
}
