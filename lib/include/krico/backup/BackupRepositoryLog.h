#pragma once

#include "Digest.h"
#include "gtest/gtest_prod.h"
#include <filesystem>
#include <ostream>
#include <istream>

namespace krico::backup {
    enum class LogEntryType : uint8_t {
        NONE = 0,
        Initialized,
    };

    namespace lengths {
        namespace LogEntry {
            static constexpr size_t Type = 1;
            static constexpr size_t Hash = DigestLength::SHA1;
            static constexpr size_t Ts = 8;
            static constexpr size_t TotalLength = Type + Hash + Ts;
        }

        namespace Next {
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

        namespace Next {
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
        void addInitLogEntry(const std::string &author);

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

        void addLogEntry(LogEntry &entry);

        FRIEND_TEST(BackupRepositoryLogTest, addLogEntry);
    };
}
