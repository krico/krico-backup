#include "krico/backup/BackupRepository.h"
#include "krico/backup/exception.h"
#include "krico/backup/io.h"
#include <spdlog/spdlog.h>


using namespace krico::backup;
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
      metaDir_(dir_ / METADATA),
      lock_(acquire_lock(dir_, metaDir_)),
      config_(metaDir_ / CONFIG_FILE) {
}

BackupRepository BackupRepository::initialize(const std::filesystem::path &dir) {
    fs::file_status status = STATUS(dir);
    if (status.type() != std::filesystem::file_type::directory) {
        THROW_EXCEPTION("Directory '" + dir.string() +"' doesn't exist");
    }
    const fs::path metaDir = dir / METADATA;
    status = STATUS(metaDir);
    if (status.type() != std::filesystem::file_type::not_found) {
        THROW_EXCEPTION("Directory '" + dir.string() +"' already initialized");
    }
    MKDIR(metaDir);
    return BackupRepository{dir};
}

void BackupRepository::unlock() {
    ASSERT_LOCKED();
    lock_.unlock();
}
