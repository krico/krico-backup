#include "krico/backup/TemporaryDirectory.h"
#include "krico/backup/exception.h"
#include <gtest/gtest.h>

using namespace krico::backup;
namespace fs = std::filesystem;

TEST(TemporaryDirectoryTest, default_constructor) {
    fs::path dir;
    do {
        TemporaryDirectory tmp{};
        dir = tmp.dir();
        ASSERT_TRUE(exists(dir));
        ASSERT_TRUE(is_directory(dir));
        ASSERT_TRUE(dir.filename().string().starts_with(TemporaryDirectory::DEFAULT_PREFIX));
        ASSERT_EQ(fs::canonical(fs::temp_directory_path()), fs::canonical(dir.parent_path()));
    } while (false);
    ASSERT_FALSE(exists(dir));
}

TEST(TemporaryDirectoryTest, ptr) {
    TemporaryDirectory::ptr tmp{new TemporaryDirectory};
    const fs::path dir = tmp->dir();
    ASSERT_TRUE(exists(dir));
    tmp.reset();
    ASSERT_FALSE(exists(dir));
}

TEST(TemporaryDirectoryTest, constructor) {
    const fs::path tmpDir = std::filesystem::temp_directory_path();
    int count = 0;
    fs::path baseDir;
    do {
        baseDir = tmpDir / ("TemporaryDirectoryTest-" + std::to_string(++count));
    } while (exists(baseDir));
    ASSERT_THROW(TemporaryDirectory{baseDir}, exception) << "Directory doesn't exist: " << baseDir;
    ASSERT_TRUE(create_directory(baseDir));

    fs::path dir;
    do {
        TemporaryDirectory tmp{baseDir};
        dir = tmp.dir();
        ASSERT_TRUE(exists(dir));
        ASSERT_TRUE(is_directory(dir));
        ASSERT_TRUE(dir.filename().string().starts_with(TemporaryDirectory::DEFAULT_PREFIX));
        ASSERT_EQ(fs::canonical(baseDir), fs::canonical(dir.parent_path()));
    } while (false);
    ASSERT_FALSE(exists(dir));

    do {
        TemporaryDirectory tmp{baseDir, "MyTest-"};
        dir = tmp.dir();
        ASSERT_TRUE(exists(dir));
        ASSERT_TRUE(is_directory(dir));
        ASSERT_TRUE(dir.filename().string().starts_with("MyTest-"));
        ASSERT_EQ(fs::canonical(baseDir), fs::canonical(dir.parent_path()));
    } while (false);
    ASSERT_FALSE(exists(dir));

    ASSERT_TRUE(fs::remove(baseDir));
}

TEST(TemporaryDirectoryTest, args) {
    const fs::path tmpDir = std::filesystem::temp_directory_path();
    int count = 0;
    fs::path baseDir;
    do {
        baseDir = tmpDir / ("TemporaryDirectoryTest-" + std::to_string(++count));
    } while (exists(baseDir));
    ASSERT_THROW(TemporaryDirectory{baseDir}, exception) << "Directory doesn't exist: " << baseDir;
    ASSERT_TRUE(create_directory(baseDir));

    fs::path dir;
    do {
        TemporaryDirectory tmp({baseDir});
        dir = tmp.dir();
        ASSERT_TRUE(exists(dir));
        ASSERT_TRUE(is_directory(dir));
        ASSERT_TRUE(dir.filename().string().starts_with(TemporaryDirectory::DEFAULT_PREFIX));
        ASSERT_EQ(fs::canonical(baseDir), fs::canonical(dir.parent_path()));
    } while (false);
    ASSERT_FALSE(exists(dir));

    do {
        TemporaryDirectory tmp(TemporaryDirectory::args_t{});
        dir = tmp.dir();
        ASSERT_TRUE(exists(dir));
        ASSERT_TRUE(is_directory(dir));
        ASSERT_TRUE(dir.filename().string().starts_with(TemporaryDirectory::DEFAULT_PREFIX));
        ASSERT_EQ(fs::canonical(fs::temp_directory_path()), fs::canonical(dir.parent_path()));
    } while (false);
    ASSERT_FALSE(exists(dir));

    do {
        TemporaryDirectory tmp({baseDir, "MyTest-"});
        dir = tmp.dir();
        ASSERT_TRUE(exists(dir));
        ASSERT_TRUE(is_directory(dir));
        ASSERT_TRUE(dir.filename().string().starts_with("MyTest-"));
        ASSERT_EQ(fs::canonical(baseDir), fs::canonical(dir.parent_path()));
    } while (false);
    ASSERT_FALSE(exists(dir));

    ASSERT_TRUE(fs::remove(baseDir));
}
