#include "krico/backup/BackupRepository.h"
#include <gtest/gtest.h>

#include "krico/backup/exception.h"
#include "krico/backup/TemporaryDirectory.h"

using namespace krico::backup;
namespace fs = std::filesystem;

TEST(BackupRepositoryTest, initialize) {
    const TemporaryDirectory tmp(TemporaryDirectory::args_t{.prefix = "Backup"});
    ASSERT_THROW(BackupRepository{tmp.dir()}, exception) << "Not initialized";
    BackupRepository repo{BackupRepository::initialize(tmp.dir())};
    ASSERT_EQ(tmp.dir(), repo.dir());
    ASSERT_THROW(BackupRepository::initialize(tmp.dir()), exception) << "Already initialized";
    ASSERT_THROW(BackupRepository{tmp.dir()}, exception) << "Not locked";
}

TEST(BackupRepositoryTest, unlock) {
    const TemporaryDirectory tmp(TemporaryDirectory::args_t{.prefix = "Backup"});
    BackupRepository repo1{BackupRepository::initialize(tmp.dir())};
    ASSERT_EQ(tmp.dir(), repo1.dir());
    ASSERT_THROW(BackupRepository{tmp.dir()}, exception) << "Not locked";
    repo1.unlock();
    BackupRepository repo2{tmp.dir()};
    ASSERT_EQ(tmp.dir(), repo2.dir());
    ASSERT_THROW(repo1.unlock(), exception) << "Not locked";
    repo2.unlock();
    ASSERT_THROW(repo2.unlock(), exception) << "Not locked";
}
