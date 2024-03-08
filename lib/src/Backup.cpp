#include "krico/backup/Backup.h"
#include "krico/backup/exception.h"
#include "krico/backup/io.h"
#include <spdlog/spdlog.h>
#include <format>
#include <fstream>


using namespace krico::backup;
using namespace std::chrono;
namespace fs = std::filesystem;

namespace {
    year_month_day backup_date(const year_month_day &date) {
        if (date.ok()) return date;
        const auto now = system_clock::now();
        return {floor<days>(now)};
    }

    fs::path create_backup_dir(const fs::path &target, const year_month_day &date) {
        for (uint16_t count = 0; count < 1000; ++count) {
            fs::path dir = target / std::format("{0:%Y/%m/%d}_{1:03d}", date, count);
            if (exists(dir)) continue;
            spdlog::debug("Creating backup directory [{}]", relative(dir, target).string());
            if (MKDIRS(dir)) {
                return dir;
            }
            THROW_EXCEPTION("Failed to create directory '" + dir.string() + "'");
        }
        THROW_EXCEPTION("Too many backups for " + std::format("{0:%Y-%m-%d}", date) + " (max 1000)");
    }
}

Backup::Backup(const fs::path &source, const fs::path &target, const year_month_day &date)
    : digest_(Digest::sha256()), source_(absolute(source)), target_(absolute(target)), date_(backup_date(date)),
      backupDir_(create_backup_dir(target_, date_)) {
}

void Backup::run() {
    statistics_ = statistics{};
    directoryDepth_ = 0;
    spdlog::debug("Backup started [Source={0}][Target={1}]",
                  source_.absolute_path().string(),
                  target_.string());
    statistics_.start_time = system_clock::now();
    backup(source_);
    statistics_.end_time = system_clock::now();
    spdlog::debug("Backup completed [Source={0}][Target={1}]: {2}",
                  source_.absolute_path().string(),
                  target_.string(),
                  std::format("{:%T}", duration_cast<nanoseconds>(statistics_.end_time - statistics_.start_time)));
}

void Backup::backup(const Directory &dir) {
    ++statistics_.directories;
    std::string prefix{};
    for (int i = 0; i < directoryDepth_; ++i) prefix += " ";
    ++directoryDepth_;
    spdlog::debug("{}Entering directory [{}]", prefix, dir.relative_path().string());

    if (const fs::path toDir = backupDir_ / dir.relative_path(); exists(toDir)) {
        spdlog::debug("Exists directory [{}]", relative(toDir, target_).string());
        if (!is_directory(toDir)) {
            THROW_EXCEPTION("Expected dir but got file '" + toDir.string() + "'");
        }
    } else {
        spdlog::debug("Creating directory [{}]", relative(toDir, target_).string());
        MKDIR(toDir);
    }
    for (const auto &entry: dir) {
        if (entry.is_directory()) {
            backup(entry.as_directory());
        } else {
            backup(entry.as_file());
        }
    }
    --directoryDepth_;
    spdlog::debug("{}Leaving directory [{}]", prefix, dir.relative_path().string());
}

void Backup::backup(const File &file) {
    ++statistics_.files;
    spdlog::debug("Backing up file [{}]", file.relative_path().string());

    const fs::path toFile = backupDir_ / file.relative_path();
    const fs::path digestFile = digest(file);

    if (!exists(digestFile)) {
        if (const fs::path digestDir = digestFile.parent_path(); !is_directory(digestDir)) {
            spdlog::debug("Creating directory [{}]", relative(digestDir, target_).string());
            MKDIRS(digestDir);
        }
        // Write to a temp file and "commit" with a rename
        const fs::path tmpDigestFile = {digestFile.parent_path() / (digestFile.filename().string() + ".tmp")};
        COPY_FILE(file.absolute_path(), tmpDigestFile);
        RENAME_FILE(tmpDigestFile, digestFile);

        ++statistics_.files_copied;
    }
    CREATE_HARD_LINK(digestFile, toFile);
    spdlog::debug("Backed up file [{}]", file.relative_path().string());
}

std::filesystem::path Backup::digest(const File &file) const {
    constexpr std::streamsize buffer_size = 8192;
    digest_.reset();
    char buffer[buffer_size];
    std::ifstream in{file.absolute_path(), std::ios::binary};
    if (!in) {
        THROW_EXCEPTION("Failed to read '" + file.absolute_path().string() + "'");
    }
    while (in.read(buffer, buffer_size)) {
        digest_.update(buffer, buffer_size);
    }
    if (in.bad()) {
        THROW_EXCEPTION("I/O error reading '" + file.absolute_path().string() + "'");
    }
    if (in.eof()) {
        digest_.update(buffer, in.gcount());
        const auto d = digest_.digest();
        return target_ / d.path();
    }
    THROW_EXCEPTION("Problem reading '" + file.absolute_path().string() + "'");
}
