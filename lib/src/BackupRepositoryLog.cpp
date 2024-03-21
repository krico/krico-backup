#include "krico/backup/BackupRepositoryLog.h"
#include "krico/backup/exception.h"
#include "krico/backup/uint8_utils.h"
#include "spdlog/spdlog.h"
#include <fstream>
#include <cstring>
#include <chrono>
#include <utility>

#include "krico/backup/io.h"
#include "krico/backup/TemporaryFile.h"

using namespace krico::backup;
using namespace std::chrono;
namespace fs = std::filesystem;

LogEntry::LogEntry() : header_{} {
}

LogEntry::LogEntry(const LogEntryType t) : LogEntry() {
    type(t);
}

LogEntryType LogEntry::type() const {
    return static_cast<LogEntryType>(header_[0]);
}

void LogEntry::type(LogEntryType type) {
    header_[0] = static_cast<uint8_t>(type);
}

Digest::result LogEntry::prev() const {
    Digest::result r{};
    std::memcpy(r.md_, header_ + offsets::LogEntry::Hash, lengths::LogEntry::Hash);
    r.len_ = lengths::LogEntry::Hash;
    return r;
}

void LogEntry::prev(const Digest::result &prev) {
    if (prev.len_ != lengths::LogEntry::Hash) {
        THROW_EXCEPTION("Invalid digest length=" + std::to_string(prev.len_)
            + " (expected " + std::to_string(lengths::LogEntry::Hash) + ")");
    }
    std::memcpy(header_ + 1, prev.md_, lengths::LogEntry::Hash);
}

uint64_t LogEntry::ts() const {
    return get_le<uint64_t>(header_ + offsets::LogEntry::Ts);
}

void LogEntry::ts(const uint64_t ts) {
    put_le(header_ + offsets::LogEntry::Ts, ts);
}

void LogEntry::update(const Digest &digest) const {
    digest.update(header_, lengths::LogEntry::TotalLength);
}

void LogEntry::write(std::ostream &out) const {
    if (!out.write(reinterpret_cast<const char *>(header_), lengths::LogEntry::TotalLength)) {
        THROW_EXCEPTION("Failed to write LogEntry");
    }
}

void LogEntry::read(std::istream &in) {
    if (!in.read(reinterpret_cast<char *>(header_), lengths::LogEntry::TotalLength)) {
        THROW_EXCEPTION("Failed to read LogEntry");
    }
}

InitLogEntry::InitLogEntry() : LogEntry(LogEntryType::Initialized) {
}

InitLogEntry::InitLogEntry(std::string author) : LogEntry(LogEntryType::Initialized), author_(std::move(author)) {
}

void InitLogEntry::update(const Digest &digest) const {
    LogEntry::update(digest);
    digest.update(author_.c_str(), author_.length());
}

void InitLogEntry::write(std::ostream &out) const {
    LogEntry::write(out);
    out << author_;
}

void InitLogEntry::read(std::istream &in) {
    LogEntry::read(in);
    author_ = std::string((std::istreambuf_iterator(in)), std::istreambuf_iterator<char>());
}

AddDirectoryLogEntry::AddDirectoryLogEntry() : LogEntry(LogEntryType::AddDirectory) {
}

AddDirectoryLogEntry::~AddDirectoryLogEntry() {
    if (buffer_) {
        std::free(buffer_);
        buffer_ = nullptr;
    }
}

AddDirectoryLogEntry::AddDirectoryLogEntry(const std::string &author,
                                           const std::string &directoryId,
                                           const std::filesystem::path &sourceDir)
    : LogEntry(LogEntryType::AddDirectory),
      authorLength_(author.length()),
      directoryIdLength_(directoryId.length()) {
    const auto sourceDirStr = sourceDir.string();
    sourceDirLength_ = sourceDirStr.length();
    buffer_ = static_cast<uint8_t *>(std::malloc(bufferSize()));
    if (!buffer_) {
        THROW_EXCEPTION("Failed to allocate buffer of size: " + std::to_string(bufferSize()));
    }
    buffer_[offsets::AddDirectoryLogEntry::AuthorLength] = authorLength_;
    put_le(buffer_ + offsets::AddDirectoryLogEntry::DirectoryIdLength, directoryIdLength_);
    put_le(buffer_ + offsets::AddDirectoryLogEntry::SourceDirLength, sourceDirLength_);
    std::memcpy(buffer_ + offsets::AddDirectoryLogEntry::Author, author.c_str(), authorLength_);
    std::memcpy(buffer_ + offsets::AddDirectoryLogEntry::Author + authorLength_,
                directoryId.c_str(), directoryIdLength_);
    std::memcpy(buffer_ + offsets::AddDirectoryLogEntry::Author + authorLength_ + directoryIdLength_,
                sourceDirStr.c_str(), sourceDirLength_);
}

void AddDirectoryLogEntry::update(const Digest &digest) const {
    LogEntry::update(digest);
    digest.update(buffer_, bufferSize());
}

void AddDirectoryLogEntry::write(std::ostream &out) const {
    LogEntry::write(out);
    if (!out.write(reinterpret_cast<const char *>(buffer_), bufferSize())) {
        THROW_EXCEPTION("Failed to write AddDirectoryLogEntry");
    }
}

void AddDirectoryLogEntry::read(std::istream &in) {
    LogEntry::read(in);
    uint8_t lengths[lengths::AddDirectoryLogEntry::Lengths];
    if (!in.read(reinterpret_cast<char *>(lengths), lengths::AddDirectoryLogEntry::Lengths)) {
        THROW_EXCEPTION("Failed to read AddDirectoryLogEntry lengths");
    }
    authorLength_ = lengths[offsets::AddDirectoryLogEntry::AuthorLength];
    directoryIdLength_ = get_le<uint16_t>(lengths + offsets::AddDirectoryLogEntry::DirectoryIdLength);
    sourceDirLength_ = get_le<uint16_t>(lengths + offsets::AddDirectoryLogEntry::SourceDirLength);

    if (buffer_) {
        std::free(buffer_);
    }
    buffer_ = static_cast<uint8_t *>(std::malloc(bufferSize()));
    if (!buffer_) {
        THROW_EXCEPTION("Failed to allocate buffer of size: " + std::to_string(bufferSize()));
    }
    std::memcpy(buffer_, lengths, lengths::AddDirectoryLogEntry::Lengths);
    if (!in.read(reinterpret_cast<char *>(buffer_ + lengths::AddDirectoryLogEntry::Lengths),
                 bufferSize() - lengths::AddDirectoryLogEntry::Lengths)) {
        THROW_EXCEPTION("Failed to read AddDirectoryLogEntry data");
    }
}

std::string_view AddDirectoryLogEntry::author() const {
    return std::string_view{
        reinterpret_cast<const char *>(buffer_
                                       + offsets::AddDirectoryLogEntry::Author),
        authorLength_
    };
}

std::string_view AddDirectoryLogEntry::directoryId() const {
    return std::string_view{
        reinterpret_cast<const char *>(buffer_
                                       + offsets::AddDirectoryLogEntry::Author
                                       + authorLength_),
        directoryIdLength_
    };
}

std::string_view AddDirectoryLogEntry::sourceDir() const {
    return std::string_view{
        reinterpret_cast<const char *>(buffer_
                                       + offsets::AddDirectoryLogEntry::Author
                                       + authorLength_
                                       + directoryIdLength_),
        sourceDirLength_
    };
}

size_t AddDirectoryLogEntry::bufferSize() const {
    return lengths::AddDirectoryLogEntry::Lengths + authorLength_ + directoryIdLength_ + sourceDirLength_;
}

RunBackupLogEntry::RunBackupLogEntry() : LogEntry(LogEntryType::RunBackup) {
}

RunBackupLogEntry::RunBackupLogEntry(const BackupSummary &summary) : LogEntry(LogEntryType::RunBackup),
                                                                     summary_(new BackupSummary(summary)) {
}

const BackupSummary &RunBackupLogEntry::summary() const {
    return *summary_;
}

void RunBackupLogEntry::update(const Digest &digest) const {
    if (!summary_)
        THROW_EXCEPTION("Cannot update RunBackupLogEntry without a summary");
    LogEntry::update(digest);
    std::stringstream ss;
    summary_->write(ss);
    const std::string written = ss.str();
    digest.update(written.c_str(), written.size());
}

void RunBackupLogEntry::write(std::ostream &out) const {
    LogEntry::write(out);
    summary_->write(out);
}

void RunBackupLogEntry::read(std::istream &in) {
    LogEntry::read(in);
    summary_ = std::make_unique<BackupSummary>(in);
}

BackupRepositoryLog::BackupRepositoryLog(std::filesystem::path dir)
    : dir_(std::move(dir)),
      headFile_(dir_ / HEAD_FILE),
      head_{},
      digest_(Digest::sha1()) {
}

void BackupRepositoryLog::putInitLogEntry(const std::string &author) {
    InitLogEntry entry{author};
    putLogEntry(entry);
}

void BackupRepositoryLog::putAddDirectoryLogEntry(const std::string &author,
                                                  const std::string &directoryId,
                                                  const std::filesystem::path &sourceDir) {
    AddDirectoryLogEntry entry{author, directoryId, sourceDir};
    putLogEntry(entry);
}

void BackupRepositoryLog::putRunBackupLogEntry(const BackupSummary &summary) {
    RunBackupLogEntry entry{summary};
    putLogEntry(entry);
}

void BackupRepositoryLog::putLogEntry(LogEntry &entry) {
    entry.prev(head());
    entry.ts(duration_cast<nanoseconds>(system_clock::now().time_since_epoch()).count());
    digest_.reset();
    entry.update(digest_);
    const auto r = digest_.digest();
    const fs::path file{dir_ / r.path(DIGEST_DIRS)};
    const fs::path dir{file.parent_path()};
    if (!is_directory(dir)) {
        MKDIRS(dir);
    }
    if (std::ofstream out{file}; out) {
        entry.write(out);
    }

    const TemporaryFile tmp(headFile_.parent_path(), HEAD_FILE);
    if (std::ofstream out{tmp.file()}; out) {
        out << r.str();
    }
    // Try to be atomic
    RENAME_FILE(tmp.file(), headFile_);
    head_ = r;
}

const LogEntry &BackupRepositoryLog::getLogEntry(const Digest::result &digest) {
    const fs::path file{dir_ / digest.path(DIGEST_DIRS)};
    if (std::ifstream in{file}; in) {
        const auto entryType = in.peek();
        if (entryType == std::ifstream::traits_type::eof()) {
            THROW_EXCEPTION("LogEntry '" + digest.str() + "' corrupt (missing type)! File '" + file.string() + "'");
        }
        auto type = static_cast<LogEntryType>(entryType);
        switch (type) {
            case LogEntryType::Initialized:
                readLogEntry_ = std::make_unique<InitLogEntry>();
                break;
            case LogEntryType::AddDirectory:
                readLogEntry_ = std::make_unique<AddDirectoryLogEntry>();
                break;
            case LogEntryType::RunBackup:
                readLogEntry_ = std::make_unique<RunBackupLogEntry>();
                break;
            default:
                spdlog::warn("Unknown LogEntryType: {}", std::to_string(entryType));
                readLogEntry_ = std::make_unique<LogEntry>();
                break;
        }
        readLogEntry_->read(in);
        return *readLogEntry_;
    }
    THROW_EXCEPTION("LogEntry '" + digest.str() + "' not found! File '" + file.string() + "'");
}

const Digest::result &BackupRepositoryLog::head() {
    if (head_.len_ == 0) {
        if (exists(headFile_)) {
            char buf[lengths::LogEntry::Hash * 2];
            const char *ptr = buf;
            if (std::ifstream in{headFile_}; in.read(buf, lengths::LogEntry::Hash * 2)) {
                while (head_.len_ < lengths::LogEntry::Hash) {
                    const char *eptr = ptr + 2;
                    if (auto [_, ec] = std::from_chars(ptr, eptr, head_.md_[head_.len_++], 16); ec != std::errc()) {
                        THROW_ERROR_CODE("Failed to parse head", std::make_error_code(ec));
                    }
                    ptr = eptr;
                }
            } else {
                THROW_EXCEPTION("Failed to read '" + headFile_.string() + "'");
            }
        } else {
            head_.len_ = lengths::LogEntry::Hash;
        }
    }
    return head_;
}
