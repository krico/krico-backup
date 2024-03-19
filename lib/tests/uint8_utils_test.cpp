#include "krico/backup/uint8_utils.h"
#include <gtest/gtest.h>

using namespace krico::backup;

TEST(uint8_utils_test, uint16_t) {
    uint8_t buf[2]{};
    ASSERT_EQ(0, get_le<uint16_t>(buf));
    put_le(buf, std::numeric_limits<uint16_t>::max());
    ASSERT_EQ(std::numeric_limits<uint16_t>::max(), get_le<uint16_t>(buf));
}

TEST(uint8_utils_test, uint64_t) {
    uint8_t buf[8]{};
    ASSERT_EQ(0, get_le<uint64_t>(buf));
    put_le(buf, std::numeric_limits<uint64_t>::max());
    ASSERT_EQ(std::numeric_limits<uint64_t>::max(), get_le<uint64_t>(buf));
}
