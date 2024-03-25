#include "krico/backup/BackupSummary.h"
#include "krico/backup/io.h"
#include "krico/backup/exception.h"
#include <spdlog/spdlog.h>
#include <istream>
#include <chrono>
#include <regex>
#include <utility>

using namespace krico::backup;
using namespace std::chrono;
namespace fs = std::filesystem;

BackupSummaryBuilder::BackupSummaryBuilder(const fs::path &metaDir,
                                           BackupDirectoryId directoryId,
                                           const year_month_day &date,
                                           std::filesystem::path backupId)
    : directoryId_(std::move(directoryId)),
      date_(date),
      backupId_(std::move(backupId)),
      summaryFile_(
          metaDir / backupId_.parent_path() / (backupId_.filename().string() + BackupSummary::SUMMARY_FILE_SUFFIX)),
      tmpFile_(summaryFile_.parent_path(), summaryFile_.filename().string()),
      out_(tmpFile_.file()),
      digest_(Digest::sha1()),
      startTime_(system_clock::now()) {
}

void BackupSummaryBuilder::addDir(const std::filesystem::path &dir) {
    ++numDirectories_;
    const auto s = dir.string();

    digest_.update(Digest::SHA1_ZERO.md_, Digest::SHA1_ZERO.len_);
    digest_.update(s.c_str(), s.length());

    out_ << "D " << s << std::endl;
}

void BackupSummaryBuilder::addCopiedFile(const std::filesystem::path &file, const Digest::result &digest) {
    ++numCopiedFiles_;
    const auto s = file.string();
    digest_.update(digest.md_, digest.len_);
    digest_.update(s.c_str(), s.length());

    out_ << "C " << digest.str() << " " << file.string() << std::endl;
}

void BackupSummaryBuilder::addHardLinkedFile(const std::filesystem::path &file, const Digest::result &digest) {
    ++numHardLinkedFiles_;
    const auto s = file.string();
    digest_.update(digest.md_, digest.len_);
    digest_.update(s.c_str(), s.length());

    out_ << "H " << digest.str() << " " << file.string() << std::endl;
}

void BackupSummaryBuilder::addSymlink(const std::filesystem::path &file, const std::filesystem::path &target) {
    ++numSymlinks_;
    const auto l = file.string();
    const auto t = target.string();
    digest_.update(l.c_str(), l.length());
    digest_.update(t.c_str(), t.length());
    out_ << "L " << l << "\t" << t << std::endl;
}

void BackupSummaryBuilder::addPreviousSymlink(const std::filesystem::path &previousTarget) {
    previousTarget_ = previousTarget;
}

void BackupSummaryBuilder::addCurrentSymlink(const std::filesystem::path &currentTarget) {
    currentTarget_ = currentTarget;
}

BackupSummary BackupSummaryBuilder::build() {
    endTime_ = system_clock::now();
    checksum_ = digest_.digest();
    // Write the final digest
    out_ << "S " << checksum_.str() << std::endl;
    out_.close();
    RENAME_FILE(tmpFile_.file(), summaryFile_);
    return BackupSummary{*this};
}

BackupSummary::BackupSummary(const BackupSummaryBuilder &builder)
    : directoryId_(builder.directoryId_),
      date_(builder.date_),
      backupId_(builder.backupId_),
      startTime_(builder.startTime_),
      endTime_(builder.endTime_),
      numDirectories_(builder.numDirectories_),
      numCopiedFiles_(builder.numCopiedFiles_),
      numHardLinkedFiles_(builder.numHardLinkedFiles_),
      numSymlinks_(builder.numSymlinks_),
      previousTarget_(builder.previousTarget_),
      currentTarget_(builder.currentTarget_),
      checksum_(builder.checksum_) {
}

BackupSummary::BackupSummary(BackupDirectoryId directoryId,
                             const year_month_day &date,
                             std::filesystem::path backupId,
                             const system_clock::time_point &startTime,
                             const system_clock::time_point &endTime,
                             const uint32_t numDirectories,
                             const uint32_t numCopiedFiles,
                             const uint32_t numHardLinkedFiles,
                             const uint32_t numSymlinks,
                             std::filesystem::path previousTarget,
                             std::filesystem::path currentTarget,
                             const Digest::result &checksum)
    : directoryId_(std::move(directoryId)),
      date_(date),
      backupId_(std::move(backupId)),
      startTime_(startTime),
      endTime_(endTime),
      numDirectories_(numDirectories),
      numCopiedFiles_(numCopiedFiles),
      numHardLinkedFiles_(numHardLinkedFiles),
      numSymlinks_(numSymlinks),
      previousTarget_(std::move(previousTarget)),
      currentTarget_(std::move(currentTarget)),
      checksum_(checksum) {
}

std::filesystem::path BackupSummary::summaryFile(const std::filesystem::path &directoryMetaDir) const {
    return directoryMetaDir / backupId_.parent_path() / (backupId_.filename().string() + SUMMARY_FILE_SUFFIX);
}

bool BackupSummary::operator==(const BackupSummary &rhs) const {
    if (this == &rhs) return true;

    return directoryId_ == rhs.directoryId_
           && date_ == rhs.date_
           && backupId_ == rhs.backupId_
           && startTime_ == rhs.startTime_
           && endTime_ == rhs.endTime_
           && numDirectories_ == rhs.numDirectories_
           && numCopiedFiles_ == rhs.numCopiedFiles_
           && numHardLinkedFiles_ == rhs.numHardLinkedFiles_
           && numSymlinks_ == rhs.numSymlinks_
           && previousTarget_ == rhs.previousTarget_
           && currentTarget_ == rhs.currentTarget_
           && checksum_ == rhs.checksum_;
}
