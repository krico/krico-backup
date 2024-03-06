#include "krico/backup/Directory.h"
#include "krico/backup/TemporaryDirectory.h"
#include <gtest/gtest.h>
#include <fstream>


using namespace krico::backup;
namespace fs = std::filesystem;

namespace {
    void touch(const fs::path &file) {
        std::ofstream out{file, std::ios_base::app};
    }
}

TEST(DirectoryTest, paths) {
    const TemporaryDirectory tmp{};
    const fs::path &base = tmp.dir();
    const fs::path file1{base / "file1"};
    touch(file1);
    const fs::path dir1{base / "dir1"};
    create_directory(dir1);
    const fs::path file2{dir1 / "file2"};
    touch(file2);

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
}

TEST(DirectoryTest, iterator) {
    const TemporaryDirectory tmp{};
    const fs::path &base = tmp.dir();
    const fs::path file1{base / "file1"};
    touch(file1);
    const fs::path dir1{base / "dir1"};
    create_directory(dir1);
    const fs::path file2{dir1 / "file2"};
    touch(file2);

    Directory dir{base};
    auto it = dir.begin();
    ASSERT_NE(dir.end(), it);
    ++it;
    ASSERT_NE(dir.end(), it);
    ++it;
    ASSERT_EQ(dir.end(), it);

    bool foundDir = false;
    bool foundFile = false;

    for (const auto &e: dir) {
        if (e.absolute_path() == dir1) {
            ASSERT_FALSE(foundDir);
            foundDir = true;
            ASSERT_FALSE(e.is_file());
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
            ASSERT_FALSE(e.is_directory());
        } else {
            GTEST_FAIL() << "Bad entry: " << e.absolute_path();
        }
    }
    ASSERT_TRUE(foundDir);
    ASSERT_TRUE(foundFile);
}
