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

    fs::path determine_backup_dir(const fs::path &target, const year_month_day &date) {
        const fs::path metaDir = target / Backup::META_DIR;
        for (uint16_t count = 0; count < 1000; ++count) {
            fs::path dir = metaDir / std::format("{0:%Y/%m/%d}_{1:03d}", date, count);
            if (exists(dir)) continue;
            return dir;
        }
        THROW_EXCEPTION("Too many backups for " + std::format("{0:%Y-%m-%d}", date) + " (max 1000)");
    }

    FileLock acquire_lock(const fs::path &target) {
        const fs::path metaDir = target / Backup::META_DIR;
        MKDIRS(metaDir);
        const fs::path file = metaDir / Backup::LOCKFILE;
        return FileLock{file};
    }
}

Backup::Backup(const fs::path &source, const fs::path &target, const year_month_day &date)
    : target_(absolute(target)), lock_(acquire_lock(target)),
      digest_(Digest::sha256()), source_(absolute(source)), date_(backup_date(date)),
      backupDir_(determine_backup_dir(target_, date_)) {
}

void Backup::run() {
    statistics_ = statistics{};
    MKDIRS(backupDir_);
    statistics_.start_time = system_clock::now();
    backup(source_);
    adjust_symlinks();
    statistics_.end_time = system_clock::now();
}

void Backup::backup(const Directory &dir) {
    ++statistics_.directories;
    std::string prefix{};

    if (const fs::path toDir = backupDir_ / dir.relative_path(); exists(toDir)) {
        if (!is_directory(toDir)) {
            THROW_EXCEPTION("Expected dir but got file '" + toDir.string() + "'");
        }
    } else {
        MKDIR(toDir);
    }
    for (const auto &entry: dir) {
        if (entry.is_directory()) {
            backup(entry.as_directory());
        } else if (entry.is_file()) {
            backup(entry.as_file());
        } else if (entry.is_symlink()) {
            backup(entry.as_symlink());
        } else {
            THROW_NOT_IMPLEMENTED("Entry type not supported");
        }
    }
}

void Backup::backup(const File &file) {
    ++statistics_.files;

    const fs::path toFile = backupDir_ / file.relative_path();
    const fs::path digestFile = digest(file);

    if (!exists(digestFile)) {
        if (const fs::path digestDir = digestFile.parent_path(); !is_directory(digestDir)) {
            MKDIRS(digestDir);
        }
        // Write to a temp file and "commit" with a rename
        const fs::path tmpDigestFile = {digestFile.parent_path() / (digestFile.filename().string() + ".tmp")};
        COPY_FILE(file.absolute_path(), tmpDigestFile);
        RENAME_FILE(tmpDigestFile, digestFile);

        ++statistics_.files_copied;
    }
    CREATE_HARD_LINK(digestFile, toFile);
}

void Backup::backup(const Symlink &symlink) {
    ++statistics_.symlinks;
    const fs::path link = backupDir_ / symlink.relative_path();
    const fs::path &target = symlink.relative_target();
    if (symlink.is_target_dir()) {
        CREATE_DIRECTORY_SYMLINK(target, link);
    } else {
        CREATE_SYMLINK(target, link);
    }
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
        return target_ / META_DIR / d.path();
    }
    THROW_EXCEPTION("Problem reading '" + file.absolute_path().string() + "'");
}

void Backup::adjust_symlinks() const {
    const fs::path previous{target_ / PREVIOUS_LINK};
    if (const auto previous_status = SYMLINK_STATUS(previous); previous_status.type() == fs::file_type::not_found) {
        spdlog::debug("not found [previous={}]", previous.string());
    } else if (previous_status.type() == fs::file_type::symlink) {
        spdlog::debug("remove [previous={}]", previous.string());
        REMOVE(previous);
    } else {
        THROW_EXCEPTION("Previous '" + previous.string() + "' is not a symlink");
    }
    const fs::path current{target_ / CURRENT_LINK};
    if (const auto current_status = SYMLINK_STATUS(current); current_status.type() == fs::file_type::not_found) {
        spdlog::debug("not found [current={}]", current.string());
    } else if (current_status.type() == fs::file_type::symlink) {
        spdlog::debug("rename [current={}][previous={}]", current.string(), previous.string());
        RENAME_FILE(current, previous);
    } else {
        THROW_EXCEPTION("Current '" + current.string() + "' is not a symlink");
    }
    const auto target = backupDir_.lexically_relative(target_);
    spdlog::debug("create symlink [current={} -> {}]", current.string(), target.string());
    CREATE_SYMLINK(backupDir_.lexically_relative(target_), current);
}
