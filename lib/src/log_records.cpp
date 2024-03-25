#include "krico/backup/log_records.h"
#include "krico/backup/exception.h"
#include <spdlog/spdlog.h>

using namespace krico::backup::records;
using namespace krico::backup;
using namespace std::chrono;

LogHeader::LogHeader()
    : buffer_(DEFAULT_BUFFER_SIZE), type_(buffer_), prev_(buffer_), ts_(buffer_), author_(buffer_) {
    fields_.reserve(4);
    fields_.push_back(&type_);
    fields_.push_back(&prev_);
    fields_.push_back(&ts_);
    fields_.push_back(&author_);
}

LogHeader::LogHeader(const LogEntryType type,
                     const Digest::result &prev,
                     const std::string_view &author) : LogHeader() {
    type_.offset(0);
    type_.set(type);

    prev_.offset(type_.end_offset());
    prev_.set(prev);

    ts_.offset(prev_.end_offset());
    ts_.set(system_clock::now());

    // Just in case someone has a VERY LONG author name ;)
    assert(ts_.end_offset() + 4 + author.length() < DEFAULT_BUFFER_SIZE);

    author_.offset(ts_.end_offset());
    author_.set(author);
}

void LogHeader::parse_offsets() const {
    const field_offset *prev = nullptr;
    for (auto *curr: fields_) {
        if (prev) curr->offset(prev->end_offset());
        else curr->offset(0);
        prev = curr;
    }
}

InitRecord::InitRecord() = default;

InitRecord::InitRecord(const Digest::result &prev, const std::string_view &author)
    : LogHeader(log_entry_type, prev, author) {
}

AddDirectoryRecord::AddDirectoryRecord(): directoryId_(buffer_), sourceDir_(buffer_) {
    add_fields();
}

AddDirectoryRecord::AddDirectoryRecord(const Digest::result &prev,
                                       const std::string_view &author,
                                       const std::string_view &directoryId,
                                       const std::filesystem::path &sourceDir)
    : LogHeader(log_entry_type, prev, author),
      directoryId_(buffer_), sourceDir_(buffer_) {
    add_fields();

    // directoryId length
    assert(author_.end_offset() + 4 + directoryId.length() < DEFAULT_BUFFER_SIZE);

    directoryId_.offset(author_.end_offset());
    directoryId_.set(directoryId);

    // sourceDir length
    assert(directoryId_.end_offset() + 4 + sourceDir.string().length() < DEFAULT_BUFFER_SIZE);

    sourceDir_.offset(directoryId_.end_offset());
    sourceDir_.set(sourceDir);
}

void AddDirectoryRecord::add_fields() {
    fields_.reserve(4 + 2);
    fields_.push_back(&directoryId_);
    fields_.push_back(&sourceDir_);
}

RunBackupRecord::RunBackupRecord()
    : directoryId_(buffer_), date_(buffer_), backupId_(buffer_), startTime_(buffer_), endTime_(buffer_),
      numDirectories_(buffer_), numCopiedFiles_(buffer_), numHardLinkedFiles_(buffer_), numSymlinks_(buffer_),
      previousTarget_(buffer_), currentTarget_(buffer_),
      checksum_(buffer_) {
    add_fields();
}

RunBackupRecord::RunBackupRecord(const Digest::result &prev,
                                 const std::string_view &author,
                                 const BackupSummary &summary)
    : LogHeader(log_entry_type, prev, author),
      directoryId_(buffer_), date_(buffer_), backupId_(buffer_), startTime_(buffer_), endTime_(buffer_),
      numDirectories_(buffer_), numCopiedFiles_(buffer_), numHardLinkedFiles_(buffer_), numSymlinks_(buffer_),
      previousTarget_(buffer_), currentTarget_(buffer_),
      checksum_(buffer_) {
    add_fields();

    // Link fields
    directoryId_.offset(author_.end_offset());
    directoryId_.set(summary.directoryId());

    date_.offset(directoryId_.end_offset());
    date_.set(summary.date());

    backupId_.offset(date_.end_offset());
    backupId_.set(summary.backupId());

    startTime_.offset(backupId_.end_offset());
    startTime_.set(summary.startTime());

    endTime_.offset(startTime_.end_offset());
    endTime_.set(summary.endTime());

    numDirectories_.offset(endTime_.end_offset());
    numDirectories_.set(summary.numDirectories());

    numCopiedFiles_.offset(numDirectories_.end_offset());
    numCopiedFiles_.set(summary.numCopiedFiles());

    numHardLinkedFiles_.offset(numCopiedFiles_.end_offset());
    numHardLinkedFiles_.set(summary.numHardLinkedFiles());

    numSymlinks_.offset(numHardLinkedFiles_.end_offset());
    numSymlinks_.set(summary.numSymlinks());

    previousTarget_.offset(numSymlinks_.end_offset());
    previousTarget_.set(summary.previousTarget());

    currentTarget_.offset(previousTarget_.end_offset());
    currentTarget_.set(summary.currentTarget());

    checksum_.offset(currentTarget_.end_offset());
    checksum_.set(summary.checksum());
}

void RunBackupRecord::add_fields() {
    fields_.reserve(4 + 12);
    fields_.push_back(&directoryId_);
    fields_.push_back(&date_);
    fields_.push_back(&backupId_);
    fields_.push_back(&startTime_);
    fields_.push_back(&endTime_);
    fields_.push_back(&numDirectories_);
    fields_.push_back(&numCopiedFiles_);
    fields_.push_back(&numHardLinkedFiles_);
    fields_.push_back(&numSymlinks_);
    fields_.push_back(&previousTarget_);
    fields_.push_back(&currentTarget_);
    fields_.push_back(&checksum_);
}

BackupSummary RunBackupRecord::summary() const {
    return BackupSummary{
        directoryId_.get(),
        date_.get(),
        backupId_.get(),
        startTime_.get(),
        endTime_.get(),
        numDirectories_.get(),
        numCopiedFiles_.get(),
        numHardLinkedFiles_.get(),
        numSymlinks_.get(),
        previousTarget_.get(),
        currentTarget_.get(),
        checksum_.get()
    };
}
