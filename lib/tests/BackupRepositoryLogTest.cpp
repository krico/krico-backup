#include "krico/backup/BackupRepositoryLog.h"
#include "krico/backup/BackupRepository.h"
#include "krico/backup/TemporaryDirectory.h"
#include <gtest/gtest.h>
#include <fstream>
#include <chrono>

using namespace std::chrono;

namespace krico::backup {
    TEST(BackupRepositoryLogTest, headHash) {
        TemporaryDirectory tmp{};
        BackupRepositoryLog log1{tmp.dir()};
        ASSERT_EQ(Digest::SHA1_ZERO, log1.head());

        const auto md = Digest::sha1();
        const std::string data{"This is a test"};
        md.update(data.c_str(), data.size());
        const auto digest = md.digest();

        if (std::ofstream out{tmp.dir() / BackupRepositoryLog::HEAD_FILE}; out) {
            out << digest.str();
        }

        BackupRepositoryLog log2{tmp.dir()};
        ASSERT_EQ(digest, log2.head());
    }

    TEST(BackupRepositoryLogTest, LogEntry) {
        LogEntry entry{};
        ASSERT_EQ(LogEntryType::NONE, entry.type());
        ASSERT_EQ(Digest::SHA1_ZERO, entry.prev());
        ASSERT_EQ(0, entry.ts());

        entry.type(LogEntryType::Initialized);

        const auto digest = Digest::sha1();
        const std::string data{"Another test"};
        digest.update(data.c_str(), data.length());
        const auto r = digest.digest();
        entry.prev(r);

        entry.ts(std::numeric_limits<uint64_t>::max());

        ASSERT_EQ(LogEntryType::Initialized, entry.type());
        ASSERT_EQ(r, entry.prev());
        ASSERT_EQ(std::numeric_limits<uint64_t>::max(), entry.ts());
    }

    TEST(BackupRepositoryLogTest, addLogEntry) {
        const TemporaryDirectory tmp{};
        BackupRepositoryLog log{tmp.dir()};
        LogEntry e1{};
        LogEntry e2{};
        LogEntry e3{};
        log.addLogEntry(e1);
        log.addLogEntry(e2);
        log.addLogEntry(e3);

        const LogEntry r3 = log.getHeadLogEntry();
        ASSERT_EQ(e3.type(), r3.type());
        ASSERT_EQ(e3.prev(), r3.prev());
        ASSERT_EQ(e3.ts(), r3.ts());

        const LogEntry r2 = log.getLogEntry(r3.prev());
        ASSERT_EQ(e2.type(), r2.type());
        ASSERT_EQ(e2.prev(), r2.prev());
        ASSERT_EQ(e2.ts(), r2.ts());

        const LogEntry r1 = log.getLogEntry(r2.prev());
        ASSERT_EQ(e1.type(), r1.type());
        ASSERT_EQ(e1.prev(), r1.prev());
        ASSERT_EQ(e1.ts(), r1.ts());
    }

    TEST(BackupRepositoryLogTest, addInitLogEntry) {
        const TemporaryDirectory tmp{};
        BackupRepositoryLog log{tmp.dir()};
        uint64_t now1 = duration_cast<nanoseconds>(system_clock::now().time_since_epoch()).count();
        log.addInitLogEntry("John Doe");

        const auto &e1 = log.getHeadLogEntry();
        ASSERT_EQ(LogEntryType::Initialized, e1.type());
        const auto &i1 = reinterpret_cast<const InitLogEntry &>(e1);
        ASSERT_EQ("John Doe", i1.author());
        ASSERT_TRUE(i1.ts() >= now1);
        uint64_t ts1 = i1.ts();
        ASSERT_EQ(ts1, i1.ts());

        uint64_t now2 = duration_cast<nanoseconds>(system_clock::now().time_since_epoch()).count();
        log.addInitLogEntry("Bob Marley");

        const auto &e2 = log.getHeadLogEntry();
        ASSERT_EQ(LogEntryType::Initialized, e2.type());
        const auto &i2 = reinterpret_cast<const InitLogEntry &>(e2);
        ASSERT_EQ("Bob Marley", i2.author());
        ASSERT_TRUE(now1 <= i2.ts());
        ASSERT_TRUE(i2.ts() >= now2);
        ASSERT_NE(Digest::SHA1_ZERO, i2.prev());

        const auto &r1 = log.getPrev(i2);
        ASSERT_EQ(LogEntryType::Initialized, r1.type());
        const auto &ir1 = reinterpret_cast<const InitLogEntry &>(r1);
        ASSERT_EQ("John Doe", ir1.author());
        ASSERT_EQ(ts1, ir1.ts());
        ASSERT_EQ(Digest::SHA1_ZERO, ir1.prev());
    }
}
