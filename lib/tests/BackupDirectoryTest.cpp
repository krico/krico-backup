#include "krico/backup/BackupDirectory.h"
#include "krico/backup/BackupRepository.h"
#include "krico/backup/TemporaryDirectory.h"
#include "krico/backup/exception.h"
#include "krico/backup/Digest.h"
#include <gtest/gtest.h>
#include <gtest/gtest_prod.h>

using namespace krico::backup;
namespace fs = std::filesystem;

namespace krico::backup {
    class BackupDirectoryTest : public testing::Test {
    protected:
        const TemporaryDirectory tmp{TemporaryDirectory::args_t{.prefix = "Backup"}};
        std::unique_ptr<BackupRepository> repository{};

        void SetUp() override {
            auto b = BackupRepository::initialize(tmp.dir());
            b.unlock();
            repository = std::make_unique<BackupRepository>(tmp.dir());
        }
    };

    TEST_F(BackupDirectoryTest, constructor) {
        const std::string idName{"test"};
        const BackupDirectoryId id{idName};
        const BackupDirectory directory{*repository, id};
        const fs::path expectedDir{repository->dir() / idName};
        ASSERT_EQ(expectedDir, directory.dir());
        const fs::path expectedMetaDir{repository->directoriesDir() / sha1_sum(idName)};
        ASSERT_EQ(expectedMetaDir, directory.metaDir());
        ASSERT_FALSE(directory.configured());
        fs::path s;
        ASSERT_THROW(s = directory.sourceDir(), exception);
    }

    TEST_F(BackupDirectoryTest, configure) {
        const std::string idName{"test"};
        const BackupDirectoryId id{idName};
        BackupDirectory directory{*repository, id};
        const auto target = directory.dir();
        ASSERT_FALSE(directory.configured());
        ASSERT_FALSE(exists(directory.dir()));
        ASSERT_FALSE(exists(directory.metaDir()));
        ASSERT_FALSE(exists(directory.metaDir()/BackupDirectory::SOURCE_FILE));
        ASSERT_FALSE(exists(directory.metaDir()/BackupDirectory::TARGET_FILE));

        TemporaryDirectory tmp{};
        const auto &sourceDir = tmp.dir();
        directory.configure(sourceDir);

        ASSERT_TRUE(exists(directory.dir()));
        ASSERT_TRUE(exists(directory.metaDir()));
        ASSERT_TRUE(exists(directory.metaDir()/BackupDirectory::SOURCE_FILE));
        ASSERT_TRUE(exists(directory.metaDir()/BackupDirectory::TARGET_FILE));

        ASSERT_TRUE(directory.configured());
        ASSERT_EQ(sourceDir, directory.sourceDir());
        ASSERT_THROW(directory.configure(sourceDir), exception);

        // Make sure reading it works
        BackupDirectory directory2{*repository, directory.metaDir()};
        ASSERT_TRUE(directory2.configured());
        ASSERT_EQ(target, directory2.dir());
        ASSERT_EQ(sourceDir, directory2.sourceDir());
        ASSERT_THROW(directory2.configure(sourceDir), exception);
    }

    TEST_F(BackupDirectoryTest, eq) {
        const BackupDirectoryId id{"foo"};
        const BackupDirectory d1{*repository, BackupDirectoryId{"foo"}};
        const BackupDirectory d2{*repository, BackupDirectoryId{"bar/../foo"}};
        const BackupDirectory d3{*repository, BackupDirectoryId{"bar"}};
        ASSERT_EQ(d1, d2);
        ASSERT_NE(d1, d3);
    }
}
