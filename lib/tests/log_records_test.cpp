#include "krico/backup/log_records.h"
#include <gtest/gtest.h>

#include "krico/backup/TemporaryDirectory.h"

using namespace krico::backup;
using namespace std::chrono;
using namespace std::chrono_literals;
namespace fs = std::filesystem;

TEST(log_records_test, LogHeader) {
    const auto start = system_clock::now();
    const LogHeader header{LogEntryType::NONE, Digest::SHA1_ZERO, "John Doe"};
    const auto end = system_clock::now();
    for (int i = 0; i < 2; ++i) {
        ASSERT_EQ(LogEntryType::NONE, header.type());
        ASSERT_EQ(Digest::SHA1_ZERO, header.prev());
        ASSERT_TRUE(header.ts() >= start);
        ASSERT_TRUE(header.ts() <= end);
        ASSERT_EQ("John Doe", header.author());
        ASSERT_EQ(38, header.end_offset());

        header.parse_offsets();
    }
    const LogHeader read{};
    std::memcpy(read.buffer().ptr(), header.buffer().ptr(), header.end_offset());
    read.parse_offsets();
    ASSERT_EQ(header.type(), read.type());
    ASSERT_EQ(header.prev(), read.prev());
    ASSERT_EQ(header.ts(), read.ts());
    ASSERT_EQ(header.author(), read.author());
    ASSERT_EQ(header.end_offset(), read.end_offset());
}

TEST(log_records_test, InitRecord) {
    const auto start = system_clock::now();
    const InitRecord init{Digest::SHA1_ZERO, "John Doe"};
    const auto end = system_clock::now();
    ASSERT_EQ(LogEntryType::Initialized, init.type());
    ASSERT_EQ(Digest::SHA1_ZERO, init.prev());
    ASSERT_TRUE(init.ts() >= start);
    ASSERT_TRUE(init.ts() <= end);
    ASSERT_EQ("John Doe", init.author());

    const InitRecord read{};
    std::memcpy(read.buffer().ptr(), init.buffer().ptr(), init.end_offset());
    read.parse_offsets();
    ASSERT_EQ(init.type(), read.type());
    ASSERT_EQ(init.prev(), read.prev());
    ASSERT_EQ(init.ts(), read.ts());
    ASSERT_EQ(init.author(), read.author());
    ASSERT_EQ(init.end_offset(), read.end_offset());
}

TEST(log_records_test, AddDirectoryRecord) {
    const auto start = system_clock::now();
    const AddDirectoryRecord add{Digest::SHA1_ZERO, "John Doe", "Dir", "/src/path"};
    const auto end = system_clock::now();

    for (int i = 0; i < 2; ++i) {
        ASSERT_EQ(LogEntryType::AddDirectory, add.type());
        ASSERT_EQ(Digest::SHA1_ZERO, add.prev());
        ASSERT_TRUE(add.ts() >= start);
        ASSERT_TRUE(add.ts() <= end);
        ASSERT_EQ("John Doe", add.author());
        ASSERT_EQ("Dir", add.directoryId());
        ASSERT_EQ(fs::path{"/src/path"}, add.sourceDir());
        ASSERT_EQ(52, add.end_offset());

        add.parse_offsets();
    }

    const AddDirectoryRecord read{};
    std::memcpy(read.buffer().ptr(), add.buffer().ptr(), add.end_offset());
    read.parse_offsets();
    ASSERT_EQ(add.type(), read.type());
    ASSERT_EQ(add.prev(), read.prev());
    ASSERT_EQ(add.ts(), read.ts());
    ASSERT_EQ(add.author(), read.author());
    ASSERT_EQ(add.directoryId(), read.directoryId());
    ASSERT_EQ(add.sourceDir(), read.sourceDir());
    ASSERT_EQ(add.end_offset(), read.end_offset());
}

TEST(log_records_test, RunBackupRecord) {
    const TemporaryDirectory tmp{};
    fs::path directoryMetaDir{tmp.dir() / "dirMeta"};
    fs::create_directory(directoryMetaDir);
    BackupSummaryBuilder builder{
        directoryMetaDir, BackupDirectoryId{""}, year_month_day{1976y, July, 15d}, fs::path{"1"}
    };
    auto summary = builder.build();
    const auto start = system_clock::now();
    RunBackupRecord run{Digest::SHA1_ZERO, "John Doe", summary};
    const auto end = system_clock::now();
    for (int i = 0; i < 2; ++i) {
        ASSERT_EQ(LogEntryType::RunBackup, run.type());
        ASSERT_EQ(Digest::SHA1_ZERO, run.prev());
        ASSERT_TRUE(run.ts() >= start);
        ASSERT_TRUE(run.ts() <= end);
        ASSERT_EQ("John Doe", run.author());
        ASSERT_EQ(summary, run.summary());
        ASSERT_EQ(99, run.end_offset());

        run.parse_offsets();
    }

    const RunBackupRecord read{};
    std::memcpy(read.buffer().ptr(), run.buffer().ptr(), run.end_offset());
    read.parse_offsets();
    ASSERT_EQ(run.type(), read.type());
    ASSERT_EQ(run.prev(), read.prev());
    ASSERT_EQ(run.ts(), read.ts());
    ASSERT_EQ(run.author(), read.author());
    ASSERT_EQ(run.summary(), read.summary());
    ASSERT_EQ(run.end_offset(), read.end_offset());
}
