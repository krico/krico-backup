#include "krico/backup/BackupRepository.h"
#include "krico/backup/BackupDirectory.h"
#include "krico/backup/exception.h"
#include "krico/backup/io.h"
#include <spdlog/spdlog.h>


using namespace krico::backup;
using namespace std::chrono;
namespace fs = std::filesystem;

#define ASSERT_LOCKED() do {                                                              \
  if (!lock_.locked()) THROW_EXCEPTION("Directory '" + dir_.string( ) + "' is UNLOCKED"); \
} while(false)

namespace {
    FileLock acquire_lock(const fs::path &dir, const fs::path &metaDir) {
        const fs::file_status status = STATUS(metaDir);
        if (status.type() != std::filesystem::file_type::directory) {
            THROW_EXCEPTION("Directory '" + dir.string() +"' is not a krico-backup directory");
        }
        if (auto lock = FileLock::try_lock(metaDir / BackupRepository::LOCK_FILE); lock) {
            return std::move(lock.value());
        }
        THROW_EXCEPTION("Directory '" + dir.string() +"' is locked by another process");
    }
}

BackupRepository::BackupRepository(const std::filesystem::path &dir)
    : dir_(absolute(dir)),
      metaDir_(dir_ / METADATA_DIR),
      lock_(acquire_lock(dir_, metaDir_)),
      config_(metaDir_ / CONFIG_FILE),
      directoriesDir_(metaDir_ / DIRECTORIES_DIR) {
}

BackupRepository BackupRepository::initialize(const std::filesystem::path &dir) {
    fs::file_status status = STATUS(dir);
    if (status.type() != std::filesystem::file_type::directory) {
        THROW_EXCEPTION("Directory '" + dir.string() +"' doesn't exist");
    }
    const fs::path metaDir = dir / METADATA_DIR;
    status = STATUS(metaDir);
    if (status.type() != std::filesystem::file_type::not_found) {
        THROW_EXCEPTION("Directory '" + dir.string() +"' already initialized");
    }
    MKDIR(metaDir);
    BackupRepository repo{dir};
    if (!exists(repo.directoriesDir())) {
        MKDIR(repo.directoriesDir());
    }
    repo.config().set(METADATA_SECTION, "", "init-ts", std::format("{}", system_clock::now()));
    return repo;
}

BackupConfig &BackupRepository::config() {
    ASSERT_LOCKED();
    return config_;
}

void BackupRepository::unlock() {
    ASSERT_LOCKED();
    lock_.unlock();
}

const BackupDirectory &BackupRepository::add_directory(const fs::path &directory, const fs::path &sourceDirectory) {
    const auto dir = directory.is_relative()
                         ? absolute(dir_ / directory).lexically_normal()
                         : directory.lexically_normal();

    const auto sourceDir = sourceDirectory.is_relative()
                               ? absolute(dir_ / sourceDirectory).lexically_normal()
                               : sourceDirectory.lexically_normal();

    const auto dirStatus = STATUS(dir);
    if (dirStatus.type() != fs::file_type::not_found) {
        THROW_EXCEPTION("Directory already exists '" + directory.string() + "'");
    }

    if (!is_lexical_sub_path(dir, dir_)) {
        THROW_EXCEPTION("Directory must be a sub-path or backup repository '" + directory.string() + "'"
            + " is not sub-path of '" + dir_.string() + "'");
    }
    if (is_lexical_sub_path(dir, metaDir_)) {
        THROW_EXCEPTION("Directory cannot be a sub-path or backup repository metadir '" + directory.string() + "'"
            + " is sub-path of '" + metaDir_.string() + "'");
    }
    const auto relDir = dir.lexically_relative(dir_);

    const auto sourceDirStatus = STATUS(sourceDir);
    switch (STATUS(sourceDir).type()) {
        case fs::file_type::not_found:
            THROW_EXCEPTION("Source directory doesn't exist '" + sourceDirectory.string() + "'");
        case std::filesystem::file_type::regular:
            THROW_EXCEPTION("Source directory is a file '" + sourceDirectory.string() + "'");
        case std::filesystem::file_type::directory:
            break;
        case std::filesystem::file_type::symlink:
            THROW_EXCEPTION("Source directory is a symlink (not yet supported) '" + sourceDirectory.string() + "'");
        default:
            THROW_EXCEPTION("Source directory must be a directory '" + sourceDirectory.string() + "'");
    }
    BackupDirectoryId id{relDir};
    BackupDirectory::ptr backupDirectory{std::make_unique<BackupDirectory>(*this, id)};

    auto &directories = loadDirectories();
    if (std::ranges::find_if(directories, [&](auto &d) {
        return d->id() == backupDirectory->id();
    }) != directories.end()) {
        THROW_EXCEPTION("Duplicate directory id '" + backupDirectory->id().str() + "'");
    }
    backupDirectory->configure(sourceDir);
    auto &ret = *directories.emplace_back(std::move(backupDirectory));
    std::ranges::sort(directories, [](auto &d1, auto &d2) { return d1->id().str() < d2->id().str(); });
    return ret;
}

std::vector<const BackupDirectory *> BackupRepository::list_directories() {
    std::vector<const BackupDirectory *> ret{};
    const auto &loaded = loadDirectories();
    ret.reserve(loaded.size());
    for (const auto &d: loaded) {
        ret.emplace_back(d.get());
    }
    return ret;
}

std::vector<std::unique_ptr<BackupDirectory> > &BackupRepository::loadDirectories() {
    if (directoriesLoaded_) return directories_;

    if (const auto &dir = directoriesDir(); is_directory(dir)) {
        for (const auto &entry: fs::directory_iterator{dir}) {
            if (entry.is_directory()) {
                directories_.emplace_back(std::make_unique<BackupDirectory>(*this, entry.path()));
            }
        }
    }

    std::ranges::sort(directories_, [](auto &d1, auto &d2) { return d1->id().str() < d2->id().str(); });

    directoriesLoaded_ = true;
    return directories_;
}
