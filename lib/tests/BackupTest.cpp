#include "krico/backup/Backup.h"
#include "krico/backup/TemporaryDirectory.h"
#include <gtest/gtest.h>
#include <fstream>

using namespace krico::backup;
using namespace std::chrono;
namespace fs = std::filesystem;

TEST(BackupTest, backup) {
    const TemporaryDirectory tmpSource(TemporaryDirectory::args_t{.prefix = "Source"});
    const TemporaryDirectory tmpTarget(TemporaryDirectory::args_t{.prefix = "Target"});
    const fs::path &target = tmpTarget.dir();
    const fs::path &source = tmpSource.dir();
    const fs::path file1{source / "file1.txt"};
    const fs::path fileLink{source / "fileLink.txt"};
    do {
        std::ofstream out{file1};
        // Digest: 1294ae29913c994993ea89efd7ddae0a73fcedda0b03c17a40c4d9c64bbd36f7
        out << "Hello OpenSSL krico-backup world";
    } while (false);
    fs::create_symlink(file1, fileLink);

    year_month_day date{1976y, July, 15d};
    do {
        Backup bkp{source, target, date};
        ASSERT_EQ(target/ "1976"/ "07"/ "15_000", bkp.backup_dir());
        bkp.run();

        const fs::path backupFile = bkp.backup_dir() / "file1.txt";
        const fs::path backupFileLink = bkp.backup_dir() / "fileLink.txt";
        const fs::path backupDigest =
                target / "12/94/ae/1294ae29913c994993ea89efd7ddae0a73fcedda0b03c17a40c4d9c64bbd36f7";
        ASSERT_TRUE(exists(backupFile));
        ASSERT_TRUE(exists(backupDigest));
        ASSERT_TRUE(exists(backupFileLink));
        ASSERT_TRUE(is_symlink(backupFileLink));
        ASSERT_EQ(backupFile.filename(), read_symlink(backupFileLink));
        ASSERT_EQ(canonical(backupFile), canonical(backupFileLink));
        ASSERT_EQ(2, std::filesystem::hard_link_count(backupFile));
        ASSERT_EQ(2, std::filesystem::hard_link_count(backupDigest));
        auto stats = bkp.stats();
        ASSERT_EQ(1, stats.directories);
        ASSERT_EQ(1, stats.files);
        ASSERT_EQ(1, stats.files_copied);
    } while (false);

    do {
        Backup bkp1{source, target, date};
        ASSERT_EQ(target/ "1976"/ "07"/ "15_001", bkp1.backup_dir());
        const fs::path backupFile = bkp1.backup_dir() / "file1.txt";
        const fs::path backupFileLink = bkp1.backup_dir() / "fileLink.txt";
        const fs::path backupDigest =
                target / "12/94/ae/1294ae29913c994993ea89efd7ddae0a73fcedda0b03c17a40c4d9c64bbd36f7";
        bkp1.run();
        const fs::path backup1File = bkp1.backup_dir() / "file1.txt";
        ASSERT_TRUE(exists(backup1File));
        ASSERT_EQ(3, std::filesystem::hard_link_count(backup1File));
        ASSERT_EQ(3, std::filesystem::hard_link_count(backupFile));
        ASSERT_EQ(3, std::filesystem::hard_link_count(backupDigest));
        auto stats = bkp1.stats();
        ASSERT_EQ(1, stats.directories);
        ASSERT_EQ(1, stats.files);
        ASSERT_EQ(1, stats.symlinks);
        ASSERT_EQ(0, stats.files_copied);
        std::stringstream ss;
        ss << stats;
        ASSERT_TRUE(ss.str().starts_with("Directories  :"));
        std::cout << stats << std::endl;
    } while (false);
}
