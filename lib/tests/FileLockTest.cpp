#include "krico/backup/FileLock.h"
#include "krico/backup/exception.h"
#include "krico/backup/TemporaryFile.h"
#include <gtest/gtest.h>

using namespace krico::backup;


TEST(FileLockTest, constructor) {
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

TEST(FileLockTest, tryLock) {
    const TemporaryFile tmp(TemporaryFile::args_t{.suffix = ".lock"});
    ASSERT_TRUE(FileLock::try_lock(tmp.file()));
    ASSERT_TRUE(FileLock::try_lock(tmp.file()));
    const auto locked = FileLock::try_lock(tmp.file());
    ASSERT_TRUE(locked);
    ASSERT_FALSE(FileLock::try_lock(tmp.file()));
}
