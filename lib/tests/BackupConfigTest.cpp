#include "krico/backup/BackupConfig.h"
#include "krico/backup/TemporaryDirectory.h"
#include <gtest/gtest.h>
#include <fstream>
#include <sstream>

#include "krico/backup/TemporaryFile.h"

using namespace krico::backup;
namespace fs = std::filesystem;

TEST(BackupConfigTest, constructor) {
    const TemporaryDirectory tmp{};
    const fs::path file{tmp.dir() / "config"};
    ASSERT_FALSE(exists(file));
    BackupConfig config{file};
    ASSERT_TRUE(exists(file));
    ASSERT_EQ(file, config.file());

    std::string expected{};
    if (std::ifstream f(config.file()); f.is_open()) {
        std::stringstream ss;
        ss << f.rdbuf();
        expected = ss.str();
        ASSERT_FALSE(expected.empty());
    }
    BackupConfig config2{file};
    if (std::ifstream f(config2.file()); f.is_open()) {
        std::stringstream ss;
        ss << f.rdbuf();
        ASSERT_EQ(expected, ss.str());
    }
}

TEST(BackupConfigTest, parse) {
    const TemporaryFile tmp{};
    if (std::ofstream out{tmp.file()}) {
        out << "[one]" << std::endl;
        out << "a=b" << std::endl;
        out << "[ two ]" << std::endl;
        out << "   a   = bb    " << std::endl;
        out << "[two \"foo\"]" << std::endl;
        out << "   foo-bar   = a foo bar    " << std::endl;
    }
    BackupConfig config{tmp.file()};
    ASSERT_FALSE(config.get("one", "", "x"));
    ASSERT_TRUE(config.get("one", "", "a"));
    ASSERT_EQ("b", *config.get("one", "", "a"));
    ASSERT_EQ("b", *config.get("one", "a"));

    ASSERT_TRUE(config.get("two", "", "a"));
    ASSERT_EQ("bb", *config.get("two", "", "a"));
    ASSERT_EQ("bb", *config.get("two", "a"));

    ASSERT_FALSE(config.get("two", "foo", "foo-bar2"));
    ASSERT_TRUE(config.get("two", "foo", "foo-bar"));
    ASSERT_EQ("a foo bar", *config.get("two", "foo", "foo-bar"));
}

TEST(BackupConfigTest, set) {
    const TemporaryFile tmp{};
    remove(tmp.file());
    BackupConfig config{tmp.file()};
    ASSERT_FALSE(config.get("ab","cd"));

    config.set("ab", "", "cd", "bob");
    config.set("ab", "", "ef", "john");
    config.set("ab", "yy", "cd", "joe");

    ASSERT_TRUE(config.get("ab","cd"));
    ASSERT_TRUE(config.get("ab","ef"));
    ASSERT_TRUE(config.get("ab","yy","cd"));
    ASSERT_EQ("bob", *config.get("ab","cd"));
    ASSERT_EQ("john", *config.get("ab","ef"));
    ASSERT_EQ("joe", *config.get("ab","yy", "cd"));
    BackupConfig config2{tmp.file()};
    ASSERT_TRUE(config2.get("ab","cd"));
    ASSERT_EQ("bob", *config2.get("ab","cd"));

    config.set("ab", "", "cd", "bob1");
    config.set("ab", "", "ef", "john1");
    config.set("ab", "yy", "cd", "joe1");

    ASSERT_TRUE(config.get("ab","cd"));
    ASSERT_TRUE(config.get("ab","ef"));
    ASSERT_TRUE(config.get("ab","yy","cd"));
    ASSERT_EQ("bob1", *config.get("ab","cd"));
    ASSERT_EQ("john1", *config.get("ab","ef"));
    ASSERT_EQ("joe1", *config.get("ab","yy", "cd"));
}
