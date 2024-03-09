#include "krico/backup/Directory.h"
#include "krico/backup/TemporaryDirectory.h"
#include <gtest/gtest.h>
#include <fstream>

#include "spdlog/spdlog.h"


using namespace krico::backup;
namespace fs = std::filesystem;

namespace {
    class DirectoryTest : public testing::Test {
    protected:
        const TemporaryDirectory tmp{};
        const fs::path base{tmp.dir()};
        const fs::path file1{base / "file1"};
        const fs::path dir1{base / "dir1"};
        const fs::path file2{dir1 / "file2"};
        const fs::path dirLink{base / "dirLink"};
        const fs::path fileLink{base / "fileLink"};

        void SetUp() override {
            spdlog::trace("DirectoryTest setup");
            touch(file1);
            create_directory(dir1);
            touch(file2);
            create_symlink(dir1.filename(), dirLink);
            create_symlink(file1.filename(), fileLink);
        }

        static void touch(const fs::path &file) {
            std::ofstream out{file, std::ios_base::app};
        }

        // void TearDown() override {}
    };
}

TEST_F(DirectoryTest, paths) {
    Directory startDirectory{base};
    ASSERT_EQ(base, startDirectory.absolute_path());
    ASSERT_EQ(".", startDirectory.relative_path());

    File file{startDirectory, file1};
    ASSERT_EQ(file1, file.absolute_path());
    ASSERT_EQ("file1", file.relative_path());

    Directory dir{startDirectory, dir1};
    ASSERT_EQ(dir1, dir.absolute_path());
    ASSERT_EQ("dir1", dir.relative_path());

    File dirFile = File{dir, file2};
    ASSERT_EQ(file2, dirFile.absolute_path());
    ASSERT_EQ("dir1/file2", dirFile.relative_path());

    const Symlink dirSymlink = Symlink(dir, dirLink);
    ASSERT_EQ(dirLink, dirSymlink.absolute_path());
    ASSERT_EQ("dirLink", dirSymlink.relative_path());
    ASSERT_EQ("dir1", dirSymlink.target());
    ASSERT_TRUE(dirSymlink.is_target_dir());

    const Symlink fileSymlink = Symlink(dir, fileLink);
    ASSERT_EQ(fileLink, fileSymlink.absolute_path());
    ASSERT_EQ("fileLink", fileSymlink.relative_path());
    ASSERT_EQ("file1", fileSymlink.target());
    ASSERT_FALSE(fileSymlink.is_target_dir());
}

TEST_F(DirectoryTest, iterator) {
    Directory dir{base};
    auto it = dir.begin();
    ASSERT_NE(dir.end(), it);
    ++it;
    ASSERT_NE(dir.end(), it);
    ++it;
    ASSERT_NE(dir.end(), it);
    ++it;
    ASSERT_NE(dir.end(), it);
    ++it;
    ASSERT_EQ(dir.end(), it);

    bool foundDir = false;
    bool foundFile = false;
    bool foundDirLink = false;
    bool foundFileLink = false;

    for (const auto &e: dir) {
        if (e.absolute_path() == dir1) {
            ASSERT_FALSE(foundDir);
            foundDir = true;
            ASSERT_FALSE(e.is_file());
            ASSERT_FALSE(e.is_symlink());
            ASSERT_TRUE(e.is_directory());
            const auto &d = e.as_directory();
            auto dit = d.begin();
            ASSERT_NE(d.end(), dit);
            const auto &de = *dit;
            ASSERT_EQ(file2, de.absolute_path());
            ++dit;
            ASSERT_EQ(d.end(), dit);
        } else if (e.absolute_path() == file1) {
            ASSERT_FALSE(foundFile);
            foundFile = true;
            ASSERT_TRUE(e.is_file());
            ASSERT_FALSE(e.is_symlink());
            ASSERT_FALSE(e.is_directory());
        } else if (e.absolute_path() == dirLink) {
            ASSERT_FALSE(foundDirLink);
            foundDirLink = true;
            ASSERT_TRUE(e.is_symlink());
            ASSERT_FALSE(e.is_file());
            ASSERT_FALSE(e.is_directory());
        } else if (e.absolute_path() == fileLink) {
            ASSERT_FALSE(foundFileLink);
            foundFileLink = true;
            ASSERT_TRUE(e.is_symlink());
            ASSERT_FALSE(e.is_file());
            ASSERT_FALSE(e.is_directory());
        } else {
            GTEST_FAIL() << "Bad entry: " << e.absolute_path();
        }
    }
    ASSERT_TRUE(foundDir);
    ASSERT_TRUE(foundFile);
    ASSERT_TRUE(foundDirLink);
    ASSERT_TRUE(foundFileLink);
}

TEST_F(DirectoryTest, Symlink) {
}
