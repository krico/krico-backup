#include "krico/backup/BackupRepositoryLog.h"
#include "krico/backup/exception.h"
#include "krico/backup/io.h"
#include "krico/backup/TemporaryFile.h"
#include "spdlog/spdlog.h"
#include <fstream>
#include <cstring>
#include <chrono>
#include <utility>


using namespace krico::backup;
using namespace std::chrono;
namespace fs = std::filesystem;

BackupRepositoryLog::BackupRepositoryLog(std::filesystem::path dir)
    : dir_(std::move(dir)),
      headFile_(dir_ / HEAD_FILE),
      head_{},
      digest_(Digest::sha1()) {
}

void BackupRepositoryLog::putInitRecord(const std::string &author) {
    InitRecord entry{head(), author};
    putRecord(entry);
}

void BackupRepositoryLog::putAddDirectoryRecord(const std::string &author,
                                                const std::string &directoryId,
                                                const std::filesystem::path &sourceDir) {
    AddDirectoryRecord entry{head(), author, directoryId, sourceDir};
    putRecord(entry);
}

void BackupRepositoryLog::putRunBackupRecord(const std::string &author, const BackupSummary &summary) {
    RunBackupRecord entry{head(), author, summary};
    putRecord(entry);
}

void BackupRepositoryLog::putRecord(LogHeader &entry) {
    digest_.reset();
    const size_t len = entry.end_offset();
    digest_.update(entry.buffer().ptr(), len);
    const auto r = digest_.digest();
    const fs::path file{dir_ / r.path(DIGEST_DIRS)};
    if (const fs::path dir{file.parent_path()}; !is_directory(dir)) {
        MKDIRS(dir);
    }
    if (std::ofstream out{file}; !(out && out.write(entry.buffer().const_cptr(), static_cast<std::streamsize>(len)))) {
        THROW_EXCEPTION("Failed to write LogEntry to '" + file.string() + "'");
    }

    const TemporaryFile tmp(headFile_.parent_path(), HEAD_FILE);
    if (std::ofstream out{tmp.file()}; !(out && out << r.str())) {
        THROW_EXCEPTION("Failed to write LogEntry hash to '" + tmp.file().string() + "'");
    }

    // Try to be atomic
    RENAME_FILE(tmp.file(), headFile_);
    head_ = r;
}

const LogHeader &BackupRepositoryLog::getRecord(const Digest::result &digest) {
    const fs::path file{dir_ / digest.path(DIGEST_DIRS)};
    auto length = FILE_SIZE(file);
    if (std::ifstream in{file}; in) {
        const auto entryType = in.peek();
        if (entryType == std::ifstream::traits_type::eof()) {
            THROW_EXCEPTION("LogHeader '" + digest.str() + "' corrupt (missing type)! File '" + file.string() + "'");
        }
        LogHeader *record;
        switch (static_cast<LogEntryType>(entryType)) {
            case InitRecord::log_entry_type:
                record = &readers_.init_;
                break;
            case AddDirectoryRecord::log_entry_type:
                record = &readers_.add_;
                break;
            case RunBackupRecord::log_entry_type:
                record = &readers_.run_;
                break;
            default:
                spdlog::warn("Unknown LogHeader: {}", std::to_string(entryType));
                record = &readers_.header_;
                break;
        }
        if (!in.read(record->buffer().cptr(), static_cast<std::streamsize>(length))) {
            THROW_EXCEPTION("Failed to read LogHeader '" + digest.str() + "'! File '" + file.string() + "'");
        }
        record->parse_offsets();
        return *record;
    }
    THROW_EXCEPTION("LogHeader '" + digest.str() + "' not found! File '" + file.string() + "'");
}

const Digest::result &BackupRepositoryLog::head() {
    if (head_.len_ == 0) {
        if (exists(headFile_)) {
            std::string line;
            if (std::ifstream in{headFile_}; std::getline(in, line)) {
                Digest::result::parse(head_, line);
            } else {
                THROW_EXCEPTION("Failed to read '" + headFile_.string() + "'");
            }
        } else {
            head_.len_ = DigestLength::SHA1;
        }
    }
    return head_;
}

std::vector<Digest::result> BackupRepositoryLog::findHash(const std::string &hash) const {
    // Just in case someone decided to change this... remind them they need to fix the logic ;)
    static_assert(DIGEST_DIRS == 1, "This method only DIGEST_DIRS == 1");

    if (hash.size() == DigestLength::SHA1) {
        Digest::result digest{};
        Digest::result::parse(digest, hash);
        const fs::path file{dir_ / digest.path(DIGEST_DIRS)};
        const auto status = STATUS(file);
        if (status.type() == fs::file_type::regular) {
            return {digest};
        }
        return {};
    }

    std::vector<Digest::result> hashes_{};

    const std::string hashPrefix{hash.size() > 2 ? hash.substr(0, 2) : hash};
    for (const auto &dirEntry: fs::directory_iterator{dir_}) {
        if (!dirEntry.is_directory()) continue;
        const std::string prefix{dirEntry.path().filename().string()};
        if (!prefix.starts_with(hashPrefix)) continue;
        for (const auto &fileEntry: fs::directory_iterator{dirEntry.path()}) {
            if (!fileEntry.is_regular_file()) continue;
            const std::string fileHash{prefix + fileEntry.path().filename().string()};
            if (fileHash.starts_with(hash)) {
                Digest::result::parse(hashes_.emplace_back(), fileHash);
            }
        }
    }

    return hashes_;
}
