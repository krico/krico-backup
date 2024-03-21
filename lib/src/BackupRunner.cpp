#include "krico/backup/BackupRunner.h"
#include "krico/backup/BackupRepository.h"
#include "krico/backup/exception.h"
#include "krico/backup/io.h"
#include <spdlog/spdlog.h>
#include <fstream>

#include "krico/backup/BackupSummary.h"

using namespace krico::backup;
using namespace std::chrono;
namespace fs = std::filesystem;

BackupRunner::BackupRunner(const BackupDirectory &directory, const year_month_day &date)
    : directory_(directory),
      date_(date.ok() ? date : year_month_day{floor<days>(system_clock::now())}),
      backupDir_(determineBackupDir(directory_, date_)),
      digest_(Digest::sha256()) {
    if (!is_directory(directory_.sourceDir())) {
        THROW_EXCEPTION("Invalid source directory '" + directory_.sourceDir().string() + "'");
    }
}

BackupSummary BackupRunner::run() {
    if (exists(backupDir_)) {
        THROW_EXCEPTION("Backup directory already exists '" + backupDir_.string() + "'");
    }
    spdlog::debug("Creating backup dir '{}'", backupDir_.string());
    MKDIRS(backupDir_);
    BackupSummaryBuilder builder{
        directory_.metaDir(),
        directory_.id(),
        date_,
        backupDir_.lexically_relative(directory_.metaDir())
    };
    const Directory source{directory_.sourceDir()};
    backup(builder, source);
    adjustSymlinks(builder);
    return builder.build();
}

std::filesystem::path BackupRunner::determineBackupDir(const BackupDirectory &directory, const year_month_day &date) {
    const auto &metaDir = directory.metaDir();
    for (uint16_t count = 0; count < 1000; ++count) {
        fs::path dir = metaDir / std::format("{0:%Y/%m%d}{1:03d}", date, count);
        if (exists(dir)) continue;
        return dir;
    }
    THROW_EXCEPTION("Too many backups for " + std::format("{0:%Y-%m-%d}", date) + " (max 1000)");
}

void BackupRunner::backup(BackupSummaryBuilder &builder, const Directory &dir) /* NOLINT(*-no-recursion) */ {
    builder.addDir(dir.relative_path());
    if (const fs::path toDir = backupDir_ / dir.relative_path(); exists(toDir)) {
        if (!is_directory(toDir)) {
            THROW_EXCEPTION("Expected dir but got file '" + toDir.string() + "'");
        }
    } else {
        MKDIR(toDir);
    }
    for (const auto &entry: dir) {
        if (entry.is_directory()) {
            backup(builder, entry.as_directory());
        } else if (entry.is_file()) {
            backup(builder, entry.as_file());
        } else if (entry.is_symlink()) {
            backup(builder, entry.as_symlink());
        } else {
            THROW_NOT_IMPLEMENTED("Entry type not supported");
        }
    }
}

void BackupRunner::backup(BackupSummaryBuilder &builder, const File &file) {
    const fs::path toFile = backupDir_ / file.relative_path();
    const auto digestResult = digest(file);
    const fs::path digestFile = directory_.repository().hardLinksDir() / digestResult.path(DIGEST_DIRS);

    if (exists(digestFile)) {
        builder.addHardLinkedFile(file.relative_path(), digestResult);
    } else {
        builder.addCopiedFile(file.relative_path(), digestResult);

        if (const fs::path digestDir = digestFile.parent_path(); !is_directory(digestDir)) {
            MKDIRS(digestDir);
        }
        // Write to a temp file and "commit" with a rename
        const fs::path tmpDigestFile = {digestFile.parent_path() / (digestFile.filename().string() + ".tmp")};
        COPY_FILE(file.absolute_path(), tmpDigestFile);
        RENAME_FILE(tmpDigestFile, digestFile);
    }
    CREATE_HARD_LINK(digestFile, toFile);
}

void BackupRunner::backup(BackupSummaryBuilder &builder, const Symlink &symlink) {
    const fs::path link = backupDir_ / symlink.relative_path();
    const fs::path &target = symlink.relative_target();
    builder.addSymlink(symlink.relative_path(), target);
    if (symlink.is_target_dir()) {
        CREATE_DIRECTORY_SYMLINK(target, link);
    } else {
        CREATE_SYMLINK(target, link);
    }
}

Digest::result BackupRunner::digest(const File &file) const {
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
        return digest_.digest();
    }
    THROW_EXCEPTION("Problem reading '" + file.absolute_path().string() + "'");
}

void BackupRunner::adjustSymlinks(BackupSummaryBuilder &builder) const {
    const fs::path previous{directory_.dir() / PREVIOUS_LINK};
    if (const auto previous_status = SYMLINK_STATUS(previous); previous_status.type() == fs::file_type::not_found) {
        spdlog::debug("not found [previous={}]", previous.string());
    } else if (previous_status.type() == fs::file_type::symlink) {
        spdlog::debug("remove [previous={}]", previous.string());
        builder.addPreviousSymlink(relative(previous, directory_.metaDir()));
        REMOVE(previous);
    } else {
        THROW_EXCEPTION("Previous '" + previous.string() + "' is not a symlink");
    }
    const fs::path current{directory_.dir() / CURRENT_LINK};
    if (const auto current_status = SYMLINK_STATUS(current); current_status.type() == fs::file_type::not_found) {
        spdlog::debug("not found [current={}]", current.string());
    } else if (current_status.type() == fs::file_type::symlink) {
        spdlog::debug("rename [current={}][previous={}]", current.string(), previous.string());
        builder.addCurrentSymlink(relative(current, directory_.metaDir()));
        RENAME_FILE(current, previous);
    } else {
        THROW_EXCEPTION("Current '" + current.string() + "' is not a symlink");
    }
    const auto target = backupDir_.lexically_relative(directory_.dir());
    spdlog::debug("create symlink [current={} -> {}]", current.string(), target.string());
    CREATE_SYMLINK(backupDir_.lexically_relative(directory_.dir()), current);
}
