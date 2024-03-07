#include "krico/backup/Backup.h"
#include "krico/backup/exception.h"
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
            if (create_directories(dir)) {
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
    statistics_.start_time = system_clock::now();
    backup(source_);
    statistics_.end_time = system_clock::now();
}

void Backup::backup(const Directory &dir) {
    ++statistics_.directories;

    if (const fs::path toDir = backupDir_ / dir.relative_path(); exists(toDir)) {
        if (!is_directory(toDir)) {
            THROW_EXCEPTION("Expected dir but got file '" + toDir.string() + "'");
        }
    } else {
        if (!create_directory(toDir)) {
            THROW_EXCEPTION("Failed to create directory '" + toDir.string() + "'");
        }
    }
    for (const auto &entry: dir) {
        if (entry.is_directory()) {
            backup(entry.as_directory());
        } else {
            backup(entry.as_file());
        }
    }
}

void Backup::backup(const File &file) {
    ++statistics_.files;

    const fs::path toFile = backupDir_ / file.relative_path();
    const fs::path digestFile = digest(file);

    if (!exists(digestFile)) {
        if (const fs::path digestDir = digestFile.parent_path(); !is_directory(digestDir)) {
            if (!create_directories(digestDir)) {
                THROW_EXCEPTION("Failed to create directory '" + digestDir.string() + "'");
            }
        }
        // Write to a temp file and "commit" with a rename
        const fs::path tmpDigestFile = {digestFile.parent_path() / (digestFile.filename().string() + ".tmp")};
        copy_file(file.absolute_path(), tmpDigestFile);
        rename(tmpDigestFile, digestFile);

        ++statistics_.files_copied;
    }
    create_hard_link(digestFile, toFile);
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
