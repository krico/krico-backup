#include "krico/backup/version.h"
#include <gtest/gtest.h>

TEST(version_test, version) {
    const std::string version{KRICO_BACKUP_VERSION};
    ASSERT_FALSE(version.empty());
    std::cout << "Version: " << version << std::endl;
}

TEST(version_test, major) {
    constexpr int major = KRICO_BACKUP_VERSION_MAJOR;
    ASSERT_TRUE(major >= 1);
    std::cout << "Major: " << major << std::endl;
}

TEST(version_test, minor) {
    constexpr int minor = KRICO_BACKUP_VERSION_MINOR;
    ASSERT_TRUE(minor >= 0);
    std::cout << "Minor: " << minor << std::endl;
}

TEST(version_test, patch) {
    constexpr int patch = KRICO_BACKUP_VERSION_PATCH;
    ASSERT_TRUE(patch >= 0);
    std::cout << "Patch: " << patch << std::endl;
}

TEST(version_test, components_match) {
    std::stringstream ss;
    ss << KRICO_BACKUP_VERSION_MAJOR << "."
            << KRICO_BACKUP_VERSION_MINOR << "."
            << KRICO_BACKUP_VERSION_PATCH;
    ASSERT_EQ(std::string(KRICO_BACKUP_VERSION), ss.str());
}

TEST(version_test, version_ts) {
    const std::string versionTs{KRICO_BACKUP_VERSION_TS};
    ASSERT_FALSE(versionTs.empty());
    std::cout << "VersionTs: " << versionTs << std::endl;
}
