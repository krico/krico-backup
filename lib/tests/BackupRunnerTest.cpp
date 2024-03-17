#include "krico/backup/BackupRunner.h"
#include "krico/backup/TemporaryDirectory.h"
#include <gtest/gtest.h>

using namespace krico::backup;
using namespace std::chrono;
using namespace std::chrono_literals;

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
    const TemporaryDirectory source{};
    auto &dir = repository->add_directory("TheTarget", source.dir());
    BackupRunner runner{dir};
    runner.run();
}
