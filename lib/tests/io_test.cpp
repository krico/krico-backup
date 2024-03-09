#include "krico/backup/io.h"
#include "krico/backup/exception.h"
#include "krico/backup/TemporaryFile.h"
#include <gtest/gtest.h>

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

TEST(io_test, FileLock) {
    const TemporaryFile tmp(TemporaryFile::args_t{.suffix = ".lock"});
    FileLock lock1{tmp.file()};
    ASSERT_TRUE(lock1.locked());
    ASSERT_THROW(FileLock{tmp.file()}, exception);
    ASSERT_TRUE(lock1.locked());
    lock1.unlock();
    ASSERT_FALSE(lock1.locked());
    FileLock lock2{tmp.file()};
    ASSERT_FALSE(lock1.locked());
    ASSERT_TRUE(lock2.locked());

    FileLock lock3{std::move(lock2)};
    ASSERT_FALSE(lock1.locked());
    ASSERT_FALSE(lock2.locked());
    ASSERT_TRUE(lock3.locked());
}

TEST(io_test, FileLock_tryLock) {
    const TemporaryFile tmp(TemporaryFile::args_t{.suffix = ".lock"});
    ASSERT_TRUE(FileLock::try_lock(tmp.file()));
    ASSERT_TRUE(FileLock::try_lock(tmp.file()));
    const auto locked = FileLock::try_lock(tmp.file());
    ASSERT_TRUE(locked);
    ASSERT_FALSE(FileLock::try_lock(tmp.file()));
}
