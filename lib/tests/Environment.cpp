#include <spdlog/spdlog.h>
#include <gtest/gtest.h>

class Environment final : public ::testing::Environment {
public:
    void SetUp() override {
        prevLevel_ = spdlog::get_level();
        spdlog::set_level(spdlog::level::debug);
        spdlog::debug("Test environment SetUp");
    }

    void TearDown() override {
        spdlog::debug("Test environment TearDown");
        spdlog::set_level(prevLevel_);
    }

private:
    spdlog::level::level_enum prevLevel_{spdlog::level::info};
};

testing::Environment *const kricoBackupEnv = AddGlobalTestEnvironment(new Environment);
