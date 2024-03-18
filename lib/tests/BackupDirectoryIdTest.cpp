#include "krico/backup/BackupDirectoryId.h"
#include "krico/backup/Digest.h"
#include <gtest/gtest.h>


using namespace krico::backup;
namespace fs = std::filesystem;

TEST(BackupDirectoryIdTest, string_constructor) {
    const BackupDirectoryId id1{"foo"};
    ASSERT_EQ("foo", id1.str());
    ASSERT_EQ(fs::path{"foo"}, id1.relative_path());
    ASSERT_EQ(fs::path{sha1_sum("foo")}, id1.id_path());

    const BackupDirectoryId id2{"foo/bar"};
    ASSERT_EQ("foo/bar", id2.str());
    ASSERT_EQ(fs::path{"foo/bar"}, id2.relative_path());
    ASSERT_EQ(fs::path{sha1_sum("foo/bar")}, id2.id_path());

    const BackupDirectoryId id3{"foo_bar"};
    ASSERT_EQ("foo_bar", id3.str());
    ASSERT_EQ(fs::path{"foo_bar"}, id3.relative_path());
    ASSERT_EQ(fs::path{sha1_sum("foo_bar")}, id3.id_path());

    const BackupDirectoryId id4{"foo/../bar/baz"};
    ASSERT_EQ("bar/baz", id4.str());
    ASSERT_EQ(fs::path{"bar/baz"}, id4.relative_path());
    ASSERT_EQ(fs::path{sha1_sum("bar/baz")}, id4.id_path());
}

TEST(BackupDirectoryIdTest, eq) {
    const BackupDirectoryId id1{"foo"};
    const BackupDirectoryId id2{"foo"};
    const BackupDirectoryId id3{"bar/../foo"};
    const BackupDirectoryId id4{"bar"};
    const BackupDirectoryId id5{"foo/../bar"};
    const BackupDirectoryId id6{"foo/../baz"};
    ASSERT_EQ(id1, id2);
    ASSERT_EQ(id1, id3);
    ASSERT_NE(id1, id4);
    ASSERT_NE(id1, id5);
    ASSERT_EQ(id4, id5);
    ASSERT_NE(id5, id6);
}
