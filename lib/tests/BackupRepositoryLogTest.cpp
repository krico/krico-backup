#include "krico/backup/BackupRepositoryLog.h"
#include "krico/backup/BackupRepository.h"
#include "krico/backup/TemporaryDirectory.h"
#include <gtest/gtest.h>
#include <fstream>
#include <chrono>

using namespace std::chrono;
namespace fs = std::filesystem;

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

    TEST(BackupRepositoryLogTest, putRecord) {
        const TemporaryDirectory tmp{};
        BackupRepositoryLog log{tmp.dir()};
        LogHeader e1{LogEntryType::NONE, log.head(), "John Doe"};
        log.putRecord(e1);
        LogHeader e2{LogEntryType::NONE, log.head(), "Bob Marley"};
        log.putRecord(e2);
        LogHeader e3{LogEntryType::NONE, log.head(), "Tom Cruise"};
        log.putRecord(e3);

        const LogHeader &r3 = log.getHeadRecord();
        ASSERT_EQ(e3.type(), r3.type());
        ASSERT_EQ(e3.prev(), r3.prev());
        ASSERT_EQ(e3.ts(), r3.ts());

        const LogHeader &r2 = log.getPrev(r3);
        ASSERT_EQ(e2.type(), r2.type());
        ASSERT_EQ(e2.prev(), r2.prev());
        ASSERT_EQ(e2.ts(), r2.ts());

        const LogHeader &r1 = log.getPrev(r2);
        ASSERT_EQ(e1.type(), r1.type());
        ASSERT_EQ(e1.prev(), r1.prev());
        ASSERT_EQ(e1.ts(), r1.ts());
    }

    TEST(BackupRepositoryLogTest, findHash) {
        const TemporaryDirectory tmp{};
        BackupRepositoryLog log{tmp.dir()};
        ASSERT_TRUE(log.findHash("").empty());

        log.putInitRecord("Jonh");
        auto firstHash = log.head();

        for (int i = 0; i < log.head().len_; ++i) {
            auto found = log.findHash(firstHash.str().substr(0, i));
            ASSERT_EQ(1, found.size());
            ASSERT_EQ(firstHash, found.at(0));
        }

        log.putInitRecord("Doe");
        auto secondHash = log.head();
        for (int i = 0; i < log.head().len_; ++i) {
            auto needle = secondHash.str().substr(0, i);
            auto found = log.findHash(needle);
            if (firstHash.str().starts_with(needle)) {
                ASSERT_EQ(2, found.size());
                if (found.at(0) == firstHash) {
                    ASSERT_EQ(firstHash, found.at(0));
                    ASSERT_EQ(secondHash, found.at(1));
                } else {
                    ASSERT_EQ(firstHash, found.at(1));
                    ASSERT_EQ(secondHash, found.at(0));
                }
            } else {
                ASSERT_EQ(1, found.size());
                ASSERT_EQ(secondHash, found.at(0));
            }
        }
    }

    TEST(BackupRepositoryLogTest, putInitRecord) {
        const TemporaryDirectory tmp{};
        BackupRepositoryLog log{tmp.dir()};
        auto now1 = system_clock::now();
        log.putInitRecord("John Doe");

        const auto &e1 = log.getHeadRecord();
        ASSERT_EQ(LogEntryType::Initialized, e1.type());
        const auto &i1 = reinterpret_cast<const InitRecord &>(e1);
        ASSERT_EQ("John Doe", i1.author());
        ASSERT_TRUE(i1.ts() >= now1);
        auto ts1 = i1.ts();
        ASSERT_EQ(ts1, i1.ts());

        auto now2 = system_clock::now();
        log.putInitRecord("Bob Marley");

        const auto &e2 = log.getHeadRecord();
        ASSERT_EQ(LogEntryType::Initialized, e2.type());
        const auto &i2 = reinterpret_cast<const InitRecord &>(e2);
        ASSERT_EQ("Bob Marley", i2.author());
        ASSERT_TRUE(now1 <= i2.ts());
        ASSERT_TRUE(i2.ts() >= now2);
        ASSERT_NE(Digest::SHA1_ZERO, i2.prev());

        const auto &r1 = log.getPrev(i2);
        ASSERT_EQ(LogEntryType::Initialized, r1.type());
        const auto &ir1 = reinterpret_cast<const InitRecord &>(r1);
        ASSERT_EQ("John Doe", ir1.author());
        ASSERT_EQ(ts1, ir1.ts());
        ASSERT_EQ(Digest::SHA1_ZERO, ir1.prev());
    }

    TEST(BackupRepositoryLogTest, putAddDirectoryRecord) {
        const TemporaryDirectory tmp{};
        BackupRepositoryLog log{tmp.dir()};
        log.putAddDirectoryRecord("John Senna", "TheBackup", "/tmp/MyTheBackup");
        const auto &e1 = log.getHeadRecord();
        ASSERT_EQ(LogEntryType::AddDirectory, e1.type());
        const auto &a1 = reinterpret_cast<const AddDirectoryRecord &>(e1);
        ASSERT_EQ("John Senna", a1.author());
        ASSERT_EQ("TheBackup", a1.directoryId());
        ASSERT_EQ("/tmp/MyTheBackup", a1.sourceDir());
    }

    TEST(BackupRepositoryLogTest, putRunBackupRecord) {
        const TemporaryDirectory tmp{};
        BackupRepositoryLog log{tmp.dir()};
        fs::path directoryMetaDir{tmp.dir() / "dirMeta"};
        fs::create_directory(directoryMetaDir);
        BackupSummaryBuilder builder{
            directoryMetaDir, BackupDirectoryId{""}, year_month_day{1976y, July, 15d}, fs::path{"1"}
        };
        auto summary = builder.build();
        log.putRunBackupRecord("John Doe", summary);
        const auto &e1 = log.getHeadRecord();
        ASSERT_EQ(LogEntryType::RunBackup, e1.type());
        const auto &a1 = reinterpret_cast<const RunBackupRecord &>(e1);
        ASSERT_EQ("John Doe", a1.author());
        ASSERT_EQ(summary.startTime(), a1.summary().startTime());
        auto expectedFile = builder.summaryFile_;
        ASSERT_EQ(expectedFile, summary.summaryFile(directoryMetaDir));
    }
}
