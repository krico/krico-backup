#include "krico/backup/io.h"
#include <gtest/gtest.h>
#include <__filesystem/filesystem_error.h>

using namespace krico::backup;
namespace fs = std::filesystem;

TEST(io_test, is_lexical_sub_path) {
    ASSERT_TRUE(is_lexical_sub_path(fs::path("/a/b"), fs::path("/a")));
    ASSERT_FALSE(is_lexical_sub_path(fs::path("/a/b"), fs::path("/b")));
    ASSERT_FALSE(is_lexical_sub_path(fs::path("/a/./b"), fs::path("/b")));
    ASSERT_TRUE(is_lexical_sub_path(fs::path(), fs::path()));
    ASSERT_TRUE(is_lexical_sub_path(fs::path("/a/b/../c/d"), fs::path("/a/c")));
    ASSERT_FALSE(is_lexical_sub_path(fs::path("/a/b/../c/d"), fs::path("/a/b")));
}

TEST(io_test, lexically_relative_symlink_target) {
    ASSERT_EQ(fs::path("bbb"), lexically_relative_symlink_target(fs::path("/a/aa/aaa"),
                  fs::path("bbb"),
                  fs::path("/a/aa")));
    ASSERT_EQ(fs::path("bbb"), lexically_relative_symlink_target(fs::path("/a/aa/aaa"),
                  fs::path("/a/aa/bbb"),
                  fs::path("/a/aa")));
    ASSERT_EQ(fs::path("/a/bb/bbb"), lexically_relative_symlink_target(fs::path("/a/aa/aaa"),
                  fs::path("/a/bb/bbb"),
                  fs::path("/a/aa")));
    ASSERT_EQ(fs::path("../bb/bbb"), lexically_relative_symlink_target(fs::path("/a/aa/aaa"),
                  fs::path("/a/bb/bbb"),
                  fs::path("/a")));
    ASSERT_EQ(fs::path("../bb/bbb"), lexically_relative_symlink_target(fs::path("/a/aa/aaa"),
                  fs::path("/a/../b/../a/bb/../cc/../bb////bbb"),
                  fs::path("/a")));
}
