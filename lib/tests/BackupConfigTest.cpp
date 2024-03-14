#include "krico/backup/BackupConfig.h"
#include "krico/backup/TemporaryDirectory.h"
#include "krico/backup/exception.h"
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
    ASSERT_TRUE(config.get("ab.cd"));
    ASSERT_TRUE(config.get("ab","ef"));
    ASSERT_TRUE(config.get("ab","yy","cd"));
    ASSERT_TRUE(config.get("ab.yy.cd"));
    ASSERT_EQ("bob", *config.get("ab","cd"));
    ASSERT_EQ("john", *config.get("ab","ef"));
    ASSERT_EQ("joe", *config.get("ab","yy", "cd"));

    BackupConfig config2{tmp.file()};
    ASSERT_TRUE(config2.get("ab","cd"));
    ASSERT_EQ("bob", *config2.get("ab","cd"));

    config2.set("ab", "", "cd", "bob1");
    config2.set("ab", "", "ef", "john1");
    config2.set("ab", "yy", "cd", "joe1");

    ASSERT_TRUE(config2.get("ab","cd"));
    ASSERT_TRUE(config2.get("ab","ef"));
    ASSERT_TRUE(config2.get("ab","yy","cd"));
    ASSERT_EQ("bob1", *config2.get("ab","cd"));
    ASSERT_EQ("john1", *config2.get("ab","ef"));
    ASSERT_EQ("joe1", *config2.get("ab","yy", "cd"));
}

TEST(BackupConfigTest, set_key) {
    const TemporaryFile tmp{};
    remove(tmp.file());
    BackupConfig config{tmp.file()};
    ASSERT_FALSE(config.get("ab.cd"));
    config.set("ab.cd", "bob");
    config.set("ab.ef", "john");
    config.set("ab.yy.cd", "joe");
    config.set("a-b.yy.cd", "doe");
    config.set("a.b.c", "abc");
    config.set("a.b", "ab");
    config.set("x.Here We Have.More.Freedom.v", "A nice value");

    ASSERT_TRUE(config.get("ab","cd"));
    ASSERT_TRUE(config.get("ab.cd"));
    ASSERT_TRUE(config.get("ab","ef"));
    ASSERT_TRUE(config.get("ab","yy","cd"));
    ASSERT_TRUE(config.get("x","Here We Have.More.Freedom","v"));
    ASSERT_TRUE(config.get("ab.yy.cd"));
    ASSERT_TRUE(config.get("a-b.yy.cd"));
    ASSERT_TRUE(config.get("a.b.c"));
    ASSERT_TRUE(config.get("a.b"));
    ASSERT_TRUE(config.get("x.Here We Have.More.Freedom.v"));
    ASSERT_EQ("bob", *config.get("ab","cd"));
    ASSERT_EQ("john", *config.get("ab","ef"));
    ASSERT_EQ("joe", *config.get("ab","yy", "cd"));
    ASSERT_EQ("doe", *config.get("a-b","yy", "cd"));
    ASSERT_EQ("abc", *config.get("a","b", "c"));
    ASSERT_EQ("ab", *config.get("a","", "b"));
    ASSERT_EQ("A nice value", *config.get("x","Here We Have.More.Freedom","v"));

    BackupConfig config2{tmp.file()};
    ASSERT_TRUE(config2.get("ab","cd"));
    ASSERT_EQ("bob", *config2.get("ab","cd"));
    ASSERT_THROW(config2.set("", "value"), exception) << "Empty key";
    ASSERT_THROW(config2.set(" a.b", "value"), exception) << "Alpha numeric";
    ASSERT_THROW(config2.set("a/b.b", "value"), exception) << "Alpha numeric";
    ASSERT_THROW(config2.set("a.", "value"), exception) << "No var";
    ASSERT_THROW(config2.set("a.b.", "value"), exception) << "No var";
    ASSERT_THROW(config2.set("a.1bc", "value"), exception) << "name must start an alphabetic";
    ASSERT_THROW(config2.set("a.x x", "value"), exception) << "alphanumeric and -";
    ASSERT_THROW(config2.set("a.x\nx.y", "value"), exception) << "newline";

    config2.set("ab.cd", "bob1");
    config2.set("ab.ef", "john1");
    config2.set("ab.yy.cd", "joe1");

    ASSERT_TRUE(config2.get("ab","cd"));
    ASSERT_TRUE(config2.get("ab","ef"));
    ASSERT_TRUE(config2.get("ab","yy","cd"));
    ASSERT_TRUE(config2.get("a-b","yy","cd"));
    ASSERT_EQ("bob1", *config2.get("ab","cd"));
    ASSERT_EQ("john1", *config2.get("ab","ef"));
    ASSERT_EQ("joe1", *config2.get("ab","yy", "cd"));
    ASSERT_EQ("doe", *config2.get("a-b","yy", "cd"));
}

TEST(BackupConfigTest, list) {
    const TemporaryFile tmp{};
    BackupConfig config{tmp.file()};
    ASSERT_TRUE(config.list().empty());
    config.set("a", "", "b", "c");
    auto &list = config.list();
    ASSERT_EQ(1, list.size());
    ASSERT_EQ(&list, &config.list());
    ASSERT_TRUE(list.contains("a.b"));
    ASSERT_EQ("c", list.at("a.b"));

    config.set("a", "b", "c", "d");
    ASSERT_EQ(2, list.size());
    ASSERT_TRUE(list.contains("a.b"));
    ASSERT_EQ("c", list.at("a.b"));
    ASSERT_TRUE(list.contains("a.b.c"));
    ASSERT_EQ("d", list.at("a.b.c"));
}
