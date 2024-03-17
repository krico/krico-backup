#include "krico/backup/BackupRepository.h"
#include <gtest/gtest.h>

#include "krico/backup/BackupDirectory.h"
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

TEST(BackupRepositoryTest, add_directory) {
    const TemporaryDirectory tmp(TemporaryDirectory::args_t{.prefix = "Backup"});
    const TemporaryDirectory src(TemporaryDirectory::args_t{.prefix = "Source"});
    BackupRepository repo{BackupRepository::initialize(tmp.dir())};
    const auto &dir1 = repo.add_directory("First", src.dir());
    ASSERT_EQ(fs::path(repo.dir()/"First"), dir1.dir());
    ASSERT_EQ(src.dir(), dir1.sourceDir());
}

TEST(BackupRepositoryTest, list_directories) {
    const TemporaryDirectory tmp(TemporaryDirectory::args_t{.prefix = "Backup"});
    const TemporaryDirectory src1(TemporaryDirectory::args_t{.prefix = "Source"});
    const TemporaryDirectory src2(TemporaryDirectory::args_t{.prefix = "Source"});
    BackupRepository repo{BackupRepository::initialize(tmp.dir())};
    ASSERT_TRUE(repo.list_directories().empty());
    const auto &dir1 = repo.add_directory("A", src1.dir());
    ASSERT_EQ(1, repo.list_directories().size());
    ASSERT_EQ(dir1, *repo.list_directories().at(0));
    const auto &dir2 = repo.add_directory("B", src2.dir());
    ASSERT_EQ(2, repo.list_directories().size());
    ASSERT_EQ(dir1, *repo.list_directories().at(0))
    << "Dir1: " << dir1.id().str() << " 0: " << repo.list_directories().at(0)->id().str();
    ASSERT_EQ(dir2, *repo.list_directories().at(1));
    repo.unlock();
    BackupRepository repo1{tmp.dir()};
    ASSERT_EQ(2, repo1.list_directories().size());
    ASSERT_EQ(dir1.id(), repo1.list_directories().at(0)->id());
    ASSERT_EQ(dir2.id(), repo1.list_directories().at(1)->id());
}
