#include "krico/backup/Digest.h"
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
}

TEST(DigestTest, md5_sum) {
    constexpr auto data = "Hello OpenSSL krico-backup world";
    ASSERT_EQ("956c693dd8533233810472f64715964c", md5_sum(data));
}
