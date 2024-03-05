#include "krico/backup/settings.h"
#include <gtest/gtest.h>

using namespace krico::backup;

TEST(settings_test, dummy) {
    settings s{};
    s = settings{"foo"};
}
