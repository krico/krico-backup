#include "krico/backup/records.h"
#include <gtest/gtest.h>

using namespace krico::backup::records;
using namespace krico::backup;
using namespace std::chrono;
using namespace std::chrono_literals;

TEST(records_test, uint8_t) {
    buffer buffer{16};
    field<uint8_t> field{buffer};
    field.offset(8);
    for (uint8_t x = std::numeric_limits<uint8_t>::min(); x < std::numeric_limits<uint8_t>::max(); ++x) {
        field.set(x);
        ASSERT_EQ(x, field.get());
        ASSERT_EQ(x, buffer.const_ptr(8)[0]);
        ASSERT_EQ(9, field.end_offset());
    }
}

TEST(records_test, uint16_t) {
    buffer buffer{16};
    field<uint16_t> field{buffer};
    field.offset(8);

    field.set(1);
    ASSERT_EQ(1, field.get());
    ASSERT_EQ(10, field.end_offset());

    field.set(std::numeric_limits<uint16_t>::max());
    ASSERT_EQ(std::numeric_limits<uint16_t>::max(), field.get());
}

TEST(records_test, uint32_t) {
    buffer buffer{16};
    field<uint32_t> field{buffer};
    field.offset(8);

    field.set(1);
    ASSERT_EQ(1, field.get());
    ASSERT_EQ(12, field.end_offset());

    field.set(std::numeric_limits<uint32_t>::max());
    ASSERT_EQ(std::numeric_limits<uint32_t>::max(), field.get());
}

TEST(records_test, uint64_t) {
    buffer buffer{16};
    field<uint64_t> field{buffer};
    field.offset(8);

    field.set(1);
    ASSERT_EQ(1, field.get());
    ASSERT_EQ(16, field.end_offset());

    field.set(std::numeric_limits<uint64_t>::max());
    ASSERT_EQ(std::numeric_limits<uint64_t>::max(), field.get());
}

TEST(records_test, digest_spec) {
    buffer buffer{DigestLength::SHA1 + 8};
    field<digest_result<DigestLength::SHA1> > field{buffer};
    field.offset(8);
    auto d = Digest::sha1();
    const std::string message{"Test"};
    d.update(message.c_str(), message.length());
    const auto r = d.digest();
    field.set(r);
    const Digest::result g = field.get();
    ASSERT_EQ(r, g);
    ASSERT_EQ(8 + DigestLength::SHA1, field.end_offset());
}

TEST(records_test, dynamic_int) {
    buffer buffer{64};
    field<dynamic_int> field{buffer};
    field.offset(8);

    uint32_t v = 1;
    field.set(v);
    ASSERT_EQ(v, field.get());
    ASSERT_EQ(9, field.end_offset());

    v = dynamic_int::MAX1;
    field.set(v);
    ASSERT_EQ(v, field.get())
      << "Expected: " << std::bitset<32>(v) << std::endl
      << "Actual  : " << std::bitset<32>(field.get());

    ASSERT_EQ(9, field.end_offset());

    ++v;
    field.set(v);
    ASSERT_EQ(v, field.get())
      << "Expected: " << std::bitset<32>(v) << std::endl
      << "Actual  : " << std::bitset<32>(field.get());
    ASSERT_EQ(10, field.end_offset());

    v = dynamic_int::MAX2;
    field.set(v);
    ASSERT_EQ(v, field.get())
      << "Expected: " << std::bitset<32>(v) << std::endl
      << "Actual  : " << std::bitset<32>(field.get());
    ASSERT_EQ(10, field.end_offset());

    ++v;
    field.set(v);
    ASSERT_EQ(v, field.get())
      << "Expected: " << std::bitset<32>(v) << std::endl
      << "Actual  : " << std::bitset<32>(field.get());
    ASSERT_EQ(12, field.end_offset());

    v = dynamic_int::MAX3;
    field.set(v);
    ASSERT_EQ(v, field.get());
    ASSERT_EQ(12, field.end_offset());
}

TEST(records_test, string_view) {
    buffer buffer{32768};
    field<std::string_view> field{buffer};
    field.offset(8);

    const std::string small{"John Doe"};
    field.set(small);
    ASSERT_EQ(small, field.get());
    ASSERT_EQ(8 + 1 + small.length(), field.end_offset());

    std::stringstream ss;
    ss << std::setw(127) << std::setfill('0') << "";
    const std::string maxSmall{ss.str()};
    field.set(maxSmall);
    ASSERT_EQ(maxSmall, field.get());
    ASSERT_EQ(8 + 1 + maxSmall.length(), field.end_offset());

    ss << "1"; // Make it require 2 bytes
    const std::string medium{ss.str()};
    field.set(medium);
    ASSERT_EQ(medium, field.get());
    ASSERT_EQ(8 + 2 + medium.length(), field.end_offset());

    ss = std::stringstream{};
    ss << std::setw(16383) << std::setfill('0') << "";
    const std::string maxMedium{ss.str()};
    field.set(maxMedium);
    ASSERT_EQ(maxMedium, field.get());
    ASSERT_EQ(8 + 2 + maxMedium.length(), field.end_offset());

    ss << "1"; // Make it require 4 bytes
    const std::string large{ss.str()};
    field.set(large);
    ASSERT_EQ(large, field.get());
    ASSERT_EQ(8 + 4 + large.length(), field.end_offset());
}

TEST(records_test, year_month_day) {
    buffer buffer{16};
    field<year_month_day> field{buffer};
    field.offset(8);

    constexpr year_month_day v{1976y, July, 15d};
    field.set(v);
    ASSERT_EQ(v, field.get());
    ASSERT_EQ(12, field.end_offset());

    constexpr year_month_day v2{};
    field.set(v2);
    ASSERT_EQ(v2, field.get());
    ASSERT_EQ(12, field.end_offset());
}

TEST(records_test, time_point) {
    buffer buffer{16};
    field<system_clock::time_point> field{buffer};
    field.offset(8);

    const system_clock::time_point v{system_clock::now()};
    field.set(v);
    ASSERT_EQ(v, field.get());
    ASSERT_EQ(16, field.end_offset());
}
