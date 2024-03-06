#include "krico/backup/TemporaryFile.h"
#include "krico/backup/exception.h"
#include <gtest/gtest.h>


using namespace krico::backup;
namespace fs = std::filesystem;

TEST(TemporaryFileTest, default_constructor) {
    fs::path file;
    do {
        const TemporaryFile tmp{};
        file = tmp.file();
        ASSERT_FALSE(file.empty());
        const std::string name = file.filename();
        ASSERT_TRUE(name.starts_with(TemporaryFile::DEFAULT_PREFIX));
        ASSERT_TRUE(name.ends_with(TemporaryFile::DEFAULT_SUFFIX));
        ASSERT_EQ(fs::canonical(std::filesystem::temp_directory_path()), fs::canonical(file.parent_path()));
        ASSERT_TRUE(exists(file));
    } while (false);
    ASSERT_FALSE(exists(file)) << "TemporaryFile should be deleted by destructor";
}

TEST(TemporaryFileTest, ptr) {
    TemporaryFile::ptr tmp{new TemporaryFile()};
    const fs::path file = tmp->file();
    ASSERT_FALSE(file.empty());
    ASSERT_TRUE(exists(file));
    tmp.reset();
    ASSERT_FALSE(exists(file)) << "TemporaryFile should be deleted by destructor";
}

TEST(TemporaryFileTest, constructor) {
    const fs::path tmpDir = std::filesystem::temp_directory_path();
    int count = 0;
    fs::path dir;
    do {
        dir = tmpDir / ("TemporaryFileTest-" + std::to_string(++count));
    } while (exists(dir));
    ASSERT_THROW(TemporaryFile{dir}, exception) << "Directory doesn't exist: " << dir;
    ASSERT_TRUE(create_directory(dir));

    fs::path file;

    do {
        TemporaryFile tmp{dir};
        file = tmp.file();
        ASSERT_FALSE(file.empty());
        const std::string name = file.filename();
        ASSERT_TRUE(name.starts_with(TemporaryFile::DEFAULT_PREFIX));
        ASSERT_TRUE(name.ends_with(TemporaryFile::DEFAULT_SUFFIX));
        ASSERT_EQ(fs::canonical(dir), fs::canonical(file.parent_path()));
        ASSERT_TRUE(exists(file));
    } while (false);
    ASSERT_FALSE(exists(file)) << "TemporaryFile should be deleted by destructor";

    do {
        TemporaryFile tmp{dir, "MyTest-"};
        file = tmp.file();
        ASSERT_FALSE(file.empty());
        const std::string name = file.filename();
        ASSERT_TRUE(name.starts_with("MyTest-"));
        ASSERT_TRUE(name.ends_with(TemporaryFile::DEFAULT_SUFFIX));
        ASSERT_EQ(fs::canonical(dir), fs::canonical(file.parent_path()));
        ASSERT_TRUE(exists(file));
    } while (false);
    ASSERT_FALSE(exists(file)) << "TemporaryFile should be deleted by destructor";

    do {
        TemporaryFile tmp{dir, "MyTest-", ".test"};
        file = tmp.file();
        ASSERT_FALSE(file.empty());
        const std::string name = file.filename();
        ASSERT_TRUE(name.starts_with("MyTest-"));
        ASSERT_TRUE(name.ends_with(".test"));
        ASSERT_EQ(fs::canonical(dir), fs::canonical(file.parent_path()));
        ASSERT_TRUE(exists(file));
    } while (false);
    ASSERT_FALSE(exists(file)) << "TemporaryFile should be deleted by destructor";

    ASSERT_TRUE(fs::remove(dir));
}

TEST(TemporaryFileTest, args) {
    const fs::path tmpDir = std::filesystem::temp_directory_path();
    int count = 0;
    fs::path dir;
    do {
        dir = tmpDir / ("TemporaryFileTest-" + std::to_string(++count));
    } while (exists(dir));
    ASSERT_THROW(TemporaryFile{dir}, exception) << "Directory doesn't exist: " << dir;
    ASSERT_TRUE(create_directory(dir));

    fs::path file;

    do {
        TemporaryFile tmp(TemporaryFile::args_t{});
        file = tmp.file();
        ASSERT_FALSE(file.empty());
        const std::string name = file.filename();
        ASSERT_TRUE(name.starts_with(TemporaryFile::DEFAULT_PREFIX));
        ASSERT_TRUE(name.ends_with(TemporaryFile::DEFAULT_SUFFIX));
        ASSERT_EQ(fs::canonical(std::filesystem::temp_directory_path()), fs::canonical(file.parent_path()));
        ASSERT_TRUE(exists(file));
    } while (false);
    ASSERT_FALSE(exists(file)) << "TemporaryFile should be deleted by destructor";

    do {
        TemporaryFile tmp({dir});
        file = tmp.file();
        ASSERT_FALSE(file.empty());
        const std::string name = file.filename();
        ASSERT_TRUE(name.starts_with(TemporaryFile::DEFAULT_PREFIX));
        ASSERT_TRUE(name.ends_with(TemporaryFile::DEFAULT_SUFFIX));
        ASSERT_EQ(fs::canonical(dir), fs::canonical(file.parent_path()));
        ASSERT_TRUE(exists(file));
    } while (false);
    ASSERT_FALSE(exists(file)) << "TemporaryFile should be deleted by destructor";

    do {
        TemporaryFile tmp({dir, "MyTest-"});
        file = tmp.file();
        ASSERT_FALSE(file.empty());
        const std::string name = file.filename();
        ASSERT_TRUE(name.starts_with("MyTest-"));
        ASSERT_TRUE(name.ends_with(TemporaryFile::DEFAULT_SUFFIX));
        ASSERT_EQ(fs::canonical(dir), fs::canonical(file.parent_path()));
        ASSERT_TRUE(exists(file));
    } while (false);
    ASSERT_FALSE(exists(file)) << "TemporaryFile should be deleted by destructor";

    do {
        TemporaryFile tmp({dir, "MyTest-", ".test"});
        file = tmp.file();
        ASSERT_FALSE(file.empty());
        const std::string name = file.filename();
        ASSERT_TRUE(name.starts_with("MyTest-"));
        ASSERT_TRUE(name.ends_with(".test"));
        ASSERT_EQ(fs::canonical(dir), fs::canonical(file.parent_path()));
        ASSERT_TRUE(exists(file));
    } while (false);
    ASSERT_FALSE(exists(file)) << "TemporaryFile should be deleted by destructor";

    ASSERT_TRUE(fs::remove(dir));
}
