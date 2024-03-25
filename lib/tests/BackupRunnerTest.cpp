#include "krico/backup/BackupRunner.h"
#include "krico/backup/BackupRepository.h"
#include "krico/backup/TemporaryDirectory.h"
#include <gtest/gtest.h>
#include <fstream>

using namespace krico::backup;
using namespace std::chrono;
using namespace std::chrono_literals;
namespace fs = std::filesystem;

namespace {
    class BackupRunnerTest : public testing::Test {
    protected:
        const TemporaryDirectory tmp{TemporaryDirectory::args_t{.prefix = "Backup"}};
        std::unique_ptr<BackupRepository> repository{};

        void SetUp() override {
            auto b = BackupRepository::initialize(tmp.dir());
            b.unlock();
            repository = std::make_unique<BackupRepository>(tmp.dir());
        }
    };
}

TEST_F(BackupRunnerTest, run) {
    const TemporaryDirectory tmpSource{};
    const fs::path &source = tmpSource.dir();
    const fs::path file1{source / "file1.txt"};
    const fs::path fileLink{source / "fileLink.txt"};
    const fs::path dir{source / "dir"};
    const fs::path file2{dir / "file2.txt"};
    const fs::path dirLink{source / "dirLink"};
    create_directory(dir);
    create_symlink(dir, dirLink);
    do {
        std::ofstream out{file1};
        // Digest: 1294ae29913c994993ea89efd7ddae0a73fcedda0b03c17a40c4d9c64bbd36f7
        out << "Hello OpenSSL krico-backup world";
        std::ofstream out2{file2};
        out2 << "Hello OpenSSL krico-backup world";
    } while (false);
    create_symlink(file1, fileLink);

    year_month_day date{1976y, July, 15d};

    auto &bd = repository->add_directory("TheTarget", source);

    BackupRunner runner{bd};
    const auto summary = runner.run();

    std::cout << summary << std::endl;

    const fs::path backupFile = runner.backupDir() / "file1.txt";
    const fs::path backupFileLink = runner.backupDir() / "fileLink.txt";
    const fs::path backupDigest = repository->hardLinksDir() /
                                  "12/94/ae29913c994993ea89efd7ddae0a73fcedda0b03c17a40c4d9c64bbd36f7";
    ASSERT_TRUE(exists(backupFile));
    ASSERT_TRUE(exists(backupDigest));
    ASSERT_TRUE(exists(backupFileLink));
    ASSERT_TRUE(is_symlink(backupFileLink));
    ASSERT_EQ(backupFile.filename(), read_symlink(backupFileLink));
    ASSERT_EQ(canonical(backupFile), canonical(backupFileLink));
    ASSERT_EQ(3, std::filesystem::hard_link_count(backupFile));
    ASSERT_EQ(3, std::filesystem::hard_link_count(backupDigest));
    // auto stats = bkp.stats();
    // ASSERT_EQ(2, stats.directories);
    // ASSERT_EQ(2, stats.files);
    // ASSERT_EQ(2, stats.symlinks);
    // ASSERT_EQ(1, stats.files_copied);

    ASSERT_FALSE(exists(bd.dir()/BackupRunner::PREVIOUS_LINK));
    ASSERT_TRUE(exists(bd.dir()/BackupRunner::CURRENT_LINK));
    ASSERT_EQ(canonical(runner.backupDir()), canonical(bd.dir()/BackupRunner::CURRENT_LINK));
}
