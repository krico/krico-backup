#include "krico/backup/Digest.h"
#include <gtest/gtest.h>

using namespace krico::backup;
namespace fs = std::filesystem;

TEST(DigestTest, sha256) {
    // $ echo -n "Hello OpenSSL krico-backup world" |sha256sum
    // 1294ae29913c994993ea89efd7ddae0a73fcedda0b03c17a40c4d9c64bbd36f7  -
    Digest d = Digest::sha256();
    constexpr auto data = "Hello OpenSSL krico-backup world";
    d.update(data, strlen(data));
    const auto r = d.digest();
    ASSERT_EQ("1294ae29913c994993ea89efd7ddae0a73fcedda0b03c17a40c4d9c64bbd36f7", r.str());
    ASSERT_EQ(fs::path{"12/94/ae/1294ae29913c994993ea89efd7ddae0a73fcedda0b03c17a40c4d9c64bbd36f7"}, r.path());
    d.reset();
    d.update(data, strlen(data));
    const auto r2 = d.digest();
    ASSERT_EQ(r, r2);
}

TEST(DigestTest, sha256_sum) {
    constexpr auto data = "Hello OpenSSL krico-backup world";
    ASSERT_EQ("1294ae29913c994993ea89efd7ddae0a73fcedda0b03c17a40c4d9c64bbd36f7", sha256_sum(data));
}

TEST(DigestTest, md5) {
    // $ echo -n "Hello OpenSSL krico-backup world" |md5sum
    // 956c693dd8533233810472f64715964c -
    Digest d = Digest::md5();
    constexpr auto data = "Hello OpenSSL krico-backup world";
    d.update(data, strlen(data));
    const auto r = d.digest();
    ASSERT_EQ("956c693dd8533233810472f64715964c", r.str());
    ASSERT_EQ(fs::path{"95/6c/69/956c693dd8533233810472f64715964c"}, r.path());
    d.reset();
    d.update(data, strlen(data));
    const auto r2 = d.digest();
    ASSERT_EQ(r, r2);
}

TEST(DigestTest, md5_sum) {
    constexpr auto data = "Hello OpenSSL krico-backup world";
    ASSERT_EQ("956c693dd8533233810472f64715964c", md5_sum(data));
}
