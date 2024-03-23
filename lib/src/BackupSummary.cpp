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

namespace {
    std::string one_line(std::istream &in) {
        std::string line;
        if (!std::getline(in, line)) {
            THROW_EXCEPTION("Failed to read line");
        }
        return line;
    }

    year_month_day one_line_date(std::istream &in) {
        std::string line;
        if (!std::getline(in, line)) {
            THROW_EXCEPTION("Failed to read date line");
        }
        // gcc doesn't recognize std::chrono::from_stream so I'm going with regex
        const std::regex pattern("^([0-9]{4})-([0-9]{2})-([0-9]{2})$");
        std::smatch matches;
        if (!std::regex_search(line, matches, pattern)) {
            THROW_EXCEPTION("Invalid date line '" + line + "'");
        }
        const year_month_day ymd{
            year(std::stoi(matches[1].str())),
            month(std::stoi(matches[2].str())),
            day(std::stoi(matches[3].str()))
        };

        return ymd;
    }

    system_clock::time_point one_line_time_point(std::istream &in) {
        std::string line;
        if (!std::getline(in, line)) {
            THROW_EXCEPTION("Failed to read time point line");
        }
        const uint64_t nanos = std::stoull(line);
        system_clock::time_point tp{duration_cast<system_clock::duration>(nanoseconds(nanos))};
        return tp;
    }

    uint32_t one_line_uint32(std::istream &in) {
        std::string line;
        if (!std::getline(in, line)) {
            THROW_EXCEPTION("Failed to read uint32 line");
        }
        return std::stoul(line);
    }

    Digest::result one_line_digest(std::istream &in) {
        std::string line;
        if (!std::getline(in, line)) {
            THROW_EXCEPTION("Failed to read Digest::result line");
        }
        Digest::result ret{};
        Digest::result::parse(ret, line);
        return ret;
    }
}

BackupSummary::BackupSummary(std::istream &in)
    : directoryId_(one_line(in)),
      date_(one_line_date(in)),
      backupId_(one_line(in)),
      startTime_(one_line_time_point(in)),
      endTime_(one_line_time_point(in)),
      numDirectories_(one_line_uint32(in)),
      numCopiedFiles_(one_line_uint32(in)),
      numHardLinkedFiles_(one_line_uint32(in)),
      numSymlinks_(one_line_uint32(in)),
      previousTarget_(one_line(in)),
      currentTarget_(one_line(in)),
      checksum_(one_line_digest(in)) {
}

std::filesystem::path BackupSummary::summaryFile(const std::filesystem::path &directoryMetaDir) const {
    return directoryMetaDir / backupId_.parent_path() / (backupId_.filename().string() + SUMMARY_FILE_SUFFIX);
}

void BackupSummary::write(std::ostream &out) const {
    out << directoryId_.str() << std::endl
            << date_ << std::endl
            << backupId_.string() << std::endl
            << duration_cast<nanoseconds>(startTime_.time_since_epoch()).count() << std::endl
            << duration_cast<nanoseconds>(endTime_.time_since_epoch()).count() << std::endl
            << numDirectories_ << std::endl
            << numCopiedFiles_ << std::endl
            << numHardLinkedFiles_ << std::endl
            << numSymlinks_ << std::endl
            << previousTarget_.string() << std::endl
            << currentTarget_.string() << std::endl
            << checksum_.str() << std::endl;
}
