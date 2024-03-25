#pragma once

#include "BackupSummary.h"
#include "records.h"
#include "LogEntryType.h"
#include <vector>

namespace krico::backup {
    class LogHeader {
    public:
        static constexpr size_t DEFAULT_BUFFER_SIZE = 8192;

        LogHeader();

        LogHeader(LogEntryType type,
                  const Digest::result &prev,
                  const std::string_view &author);

        [[nodiscard]] LogEntryType type() const { return type_.get(); }
        [[nodiscard]] Digest::result prev() const { return prev_.get(); }
        [[nodiscard]] std::chrono::system_clock::time_point ts() const { return ts_.get(); }
        [[nodiscard]] std::string_view author() const { return author_.get(); }

        void parse_offsets() const;

        [[nodiscard]] size_t end_offset() const { return fields_.back()->end_offset(); }

        [[nodiscard]] const records::buffer &buffer() const { return buffer_; }

    protected:
        records::buffer buffer_{};
        std::vector<records::field_offset *> fields_{};
        records::field<LogEntryType> type_;
        records::field<records::digest_result<DigestLength::SHA1> > prev_;
        records::field<std::chrono::system_clock::time_point> ts_;
        records::field<std::string_view> author_;
    };

    class InitRecord : public LogHeader {
    public:
        static constexpr auto log_entry_type = LogEntryType::Initialized;

        InitRecord();

        InitRecord(const Digest::result &prev, const std::string_view &author);
    };

    class AddDirectoryRecord : public LogHeader {
    public:
        static constexpr auto log_entry_type = LogEntryType::AddDirectory;

        AddDirectoryRecord();

        AddDirectoryRecord(const Digest::result &prev,
                           const std::string_view &author,
                           const std::string_view &directoryId,
                           const std::filesystem::path &sourceDir);

        [[nodiscard]] std::string_view directoryId() const { return directoryId_.get(); }
        [[nodiscard]] std::filesystem::path sourceDir() const { return sourceDir_.get(); }

    private:
        records::field<std::string_view> directoryId_;
        records::field<std::filesystem::path> sourceDir_;

        void add_fields();
    };

    class RunBackupRecord : public LogHeader {
    public:
        static constexpr auto log_entry_type = LogEntryType::RunBackup;

        RunBackupRecord();

        RunBackupRecord(const Digest::result &prev,
                        const std::string_view &author,
                        const BackupSummary &summary);

        [[nodiscard]] BackupSummary summary() const;

    private:
        records::field<BackupDirectoryId> directoryId_;
        records::field<std::chrono::year_month_day> date_;
        records::field<std::filesystem::path> backupId_;
        records::field<std::chrono::system_clock::time_point> startTime_;
        records::field<std::chrono::system_clock::time_point> endTime_;
        records::field<uint32_t> numDirectories_;
        records::field<uint32_t> numCopiedFiles_;
        records::field<uint32_t> numHardLinkedFiles_;
        records::field<uint32_t> numSymlinks_;
        records::field<std::filesystem::path> previousTarget_;
        records::field<std::filesystem::path> currentTarget_;
        records::field<records::digest_result<DigestLength::SHA1> > checksum_;

        void add_fields();
    };

    template<typename T>
    const T &log_record_cast(const LogHeader &record) {
        assert(record.type() == T::log_entry_type); // Dev mistake...
        return static_cast<const T &>(record);
    }
}
