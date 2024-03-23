#include "krico/backup/Digest.h"
#include "krico/backup/exception.h"
#include <gtest/gtest.h>

using namespace krico::backup;
namespace fs = std::filesystem;

TEST(DigestTest, sha1) {
    // $ echo -n "Hello OpenSSL krico-backup world" |sha1sum
    // da8eab09d9a8dd6b450cb2184b9d1135cc5260c9 -
    const Digest d = Digest::sha1();
    constexpr auto data = "Hello OpenSSL krico-backup world";
    d.update(data, strlen(data));
    const auto r = d.digest();
    ASSERT_FALSE(r.is_zero());
    ASSERT_EQ("da8eab09d9a8dd6b450cb2184b9d1135cc5260c9", r.str());
    ASSERT_EQ(fs::path{"da/8e/ab/09d9a8dd6b450cb2184b9d1135cc5260c9"}, r.path(3));
    d.reset();
    d.update(data, strlen(data));
    const auto r2 = d.digest();
    ASSERT_EQ(r, r2);
}

TEST(DigestTest, sha1_zero) {
    ASSERT_EQ("0000000000000000000000000000000000000000", Digest::sha1().zero().str());
    ASSERT_EQ(DigestLength::SHA1, Digest::sha1().zero().len_);
    ASSERT_EQ(DigestLength::SHA1, Digest::SHA1_ZERO.len_);
    ASSERT_EQ(Digest::SHA1_ZERO, Digest::sha1().zero());
    ASSERT_TRUE(Digest::SHA1_ZERO.is_zero());
}

TEST(DigestTest, sha1_sum) {
    constexpr auto data = "Hello OpenSSL krico-backup world";
    ASSERT_EQ("da8eab09d9a8dd6b450cb2184b9d1135cc5260c9", sha1_sum(data));
}

TEST(DigestTest, sha256) {
    // $ echo -n "Hello OpenSSL krico-backup world" |sha256sum
    // 1294ae29913c994993ea89efd7ddae0a73fcedda0b03c17a40c4d9c64bbd36f7  -
    const Digest d = Digest::sha256();
    constexpr auto data = "Hello OpenSSL krico-backup world";
    d.update(data, strlen(data));
    const auto r = d.digest();
    ASSERT_FALSE(r.is_zero());
    ASSERT_EQ("1294ae29913c994993ea89efd7ddae0a73fcedda0b03c17a40c4d9c64bbd36f7", r.str());
    ASSERT_EQ(fs::path{"12/94/ae/29913c994993ea89efd7ddae0a73fcedda0b03c17a40c4d9c64bbd36f7"}, r.path(3));
    d.reset();
    d.update(data, strlen(data));
    const auto r2 = d.digest();
    ASSERT_EQ(r, r2);
}

TEST(DigestTest, sha256_zero) {
    ASSERT_EQ("0000000000000000000000000000000000000000000000000000000000000000", Digest::sha256().zero().str());
    ASSERT_EQ(DigestLength::SHA256, Digest::sha256().zero().len_);
    ASSERT_EQ(DigestLength::SHA256, Digest::SHA256_ZERO.len_);
    ASSERT_EQ(Digest::SHA256_ZERO, Digest::sha256().zero());
    ASSERT_TRUE(Digest::SHA256_ZERO.is_zero());
}

TEST(DigestTest, sha256_sum) {
    constexpr auto data = "Hello OpenSSL krico-backup world";
    ASSERT_EQ("1294ae29913c994993ea89efd7ddae0a73fcedda0b03c17a40c4d9c64bbd36f7", sha256_sum(data));
}

TEST(DigestTest, md5) {
    // $ echo -n "Hello OpenSSL krico-backup world" |md5sum
    // 956c693dd8533233810472f64715964c -
    const Digest d = Digest::md5();
    constexpr auto data = "Hello OpenSSL krico-backup world";
    d.update(data, strlen(data));
    const auto r = d.digest();
    ASSERT_FALSE(r.is_zero());
    ASSERT_EQ("956c693dd8533233810472f64715964c", r.str());
    ASSERT_EQ(fs::path{"95/6c/69/3dd8533233810472f64715964c"}, r.path(3));
    d.reset();
    d.update(data, strlen(data));
    const auto r2 = d.digest();
    ASSERT_EQ(r, r2);
}

TEST(DigestTest, md5_zero) {
    ASSERT_EQ("00000000000000000000000000000000", Digest::md5().zero().str());
    ASSERT_EQ(DigestLength::MD5, Digest::md5().zero().len_);
    ASSERT_EQ(DigestLength::MD5, Digest::MD5_ZERO.len_);
    ASSERT_EQ(Digest::MD5_ZERO, Digest::md5().zero());
    ASSERT_TRUE(Digest::MD5_ZERO.is_zero());
}

TEST(DigestTest, md5_sum) {
    constexpr auto data = "Hello OpenSSL krico-backup world";
    ASSERT_EQ("956c693dd8533233810472f64715964c", md5_sum(data));
}

TEST(DigestTest, parse) {
    Digest::result r{};

    Digest::result::parse(r, Digest::SHA1_ZERO.str());
    ASSERT_EQ(Digest::SHA1_ZERO, r);

    Digest::result::parse(r, Digest::SHA256_ZERO.str());
    ASSERT_EQ(Digest::SHA256_ZERO, r);

    Digest::result::parse(r, Digest::MD5_ZERO.str());
    ASSERT_EQ(Digest::MD5_ZERO, r);

    const std::string val{"test"};

    const Digest sha1 = Digest::sha1();
    sha1.update(val.c_str(), val.length());
    auto expected = sha1.digest();
    Digest::result::parse(r, expected.str());
    ASSERT_EQ(expected, r);

    const Digest sha256 = Digest::sha256();
    sha256.update(val.c_str(), val.length());
    expected = sha256.digest();
    Digest::result::parse(r, expected.str());
    ASSERT_EQ(expected, r);

    const Digest md5 = Digest::md5();
    md5.update(val.c_str(), val.length());
    expected = md5.digest();
    Digest::result::parse(r, expected.str());
    ASSERT_EQ(expected, r);

    std::stringstream ss;

    ss << std::setw(2) << std::setfill('0') << "";
    Digest::result::parse(r, ss.str());
    ASSERT_TRUE(r.is_zero());
    ASSERT_EQ(1, r.len_);

    ss = {};
    ss << '0';
    ASSERT_THROW(Digest::result::parse(r, ss.str()), krico::backup::exception) << "Odd";

    ss = {};
    ss << std::setw(2 * EVP_MAX_MD_SIZE) << std::setfill('0') << "";
    Digest::result::parse(r, ss.str());
    ASSERT_TRUE(r.is_zero());
    ASSERT_EQ(EVP_MAX_MD_SIZE, r.len_);
    ss << '0';
    ASSERT_THROW(Digest::result::parse(r, ss.str()), krico::backup::exception) << "Odd";
    ss << '0';
    ASSERT_THROW(Digest::result::parse(r, ss.str()), krico::backup::exception) << "Too large";
}
