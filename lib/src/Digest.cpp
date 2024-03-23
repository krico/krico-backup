#include "krico/backup/Digest.h"
#include "krico/backup/exception.h"
#include <spdlog/spdlog.h>
#include <openssl/err.h>
#include <string>
#include <sstream>
#include <iomanip>

using namespace krico::backup;
namespace fs = std::filesystem;

static_assert(Digest::SHA1_ZERO.is_zero());
static_assert(Digest::SHA256_ZERO.is_zero());
static_assert(Digest::MD5_ZERO.is_zero());
static_assert(Digest::SHA1_ZERO != Digest::SHA256_ZERO);
static_assert(Digest::SHA1_ZERO != Digest::MD5_ZERO);
static_assert(Digest::SHA256_ZERO != Digest::SHA1_ZERO);
static_assert(Digest::SHA256_ZERO != Digest::MD5_ZERO);
static_assert(Digest::MD5_ZERO != Digest::SHA1_ZERO);
static_assert(Digest::MD5_ZERO != Digest::SHA256_ZERO);

namespace {
    struct openssl_error final : exception {
        explicit openssl_error(const std::string &msg): exception(build_error_message(msg)) {
        }

    private:
        static std::string build_error_message(const std::string &msg) {
            std::stringstream ss{};
            ss << msg << " (" << OPENSSL_VERSION_TEXT << ")";
            for (int i = 0; i < 10; ++i) {
                const auto err = ERR_get_error();
                if (err == 0) break;
                if (i == 0) ss << ": ";
                else ss << std::endl << "  ";
                if (const char *errStr = ERR_error_string(err, nullptr)) {
                    ss << errStr;
                } else {
                    ss << "(code=" << err << ")";
                }
            }

            // Clear the error queue
            ERR_clear_error();

            return ss.str();
        }
    };
}

Digest::Digest(const std::string &algorithm, const std::string &properties)
    : digest_(EVP_MD_fetch(nullptr, algorithm.c_str(), properties.c_str())),
      context_(EVP_MD_CTX_new()) {
    if (!digest_) {
        if (context_) {
            EVP_MD_CTX_free(context_);
            context_ = nullptr;
        }
        THROW(openssl_error, "Digest '"+algorithm+"'");
    }
    if (!context_) {
        EVP_MD_free(digest_);
        digest_ = nullptr;
        THROW(openssl_error, "Context");
    }
    if (!EVP_DigestInit_ex(context_, digest_, nullptr)) {
        EVP_MD_CTX_free(context_);
        context_ = nullptr;
        EVP_MD_free(digest_);
        digest_ = nullptr;
        THROW(openssl_error, "DigestInit");
    }
}

Digest::~Digest() {
    if (context_) {
        EVP_MD_CTX_free(context_);
        context_ = nullptr;
    }
    if (digest_) {
        EVP_MD_free(digest_);
        digest_ = nullptr;
    }
}

Digest Digest::sha1() {
    constexpr auto ALGORITHM = "SHA-1";
    return Digest{ALGORITHM};
}

Digest Digest::sha256() {
    constexpr auto ALGORITHM = "SHA2-256";
    return Digest{ALGORITHM};
}

Digest Digest::md5() {
    constexpr auto ALGORITHM = "MD5";
    return Digest{ALGORITHM};
}

void Digest::reset() const {
    if (!EVP_DigestInit_ex(context_, digest_, nullptr)) {
        THROW(openssl_error, "DigestInit");
    }
}

void Digest::update(const void *data, const size_t len) const {
    if (!EVP_DigestUpdate(context_, data, len)) {
        THROW(openssl_error, "DigestUpdate");
    }
}

Digest::result Digest::digest() const {
    result r{};
    if (!EVP_DigestFinal_ex(context_, r.md_, &r.len_)) {
        THROW(openssl_error, "DigestFinal");
    }
    return r;
}

Digest::result Digest::zero() const {
    result r{};
    r.len_ = EVP_MD_get_size(digest_);
    return r;
}

std::string krico::backup::sha1_sum(const std::string &str) {
    const auto md = Digest::sha1();
    md.update(str.c_str(), str.length());
    return md.digest().str();
}

std::string krico::backup::sha256_sum(const std::string &str) {
    const auto md = Digest::sha256();
    md.update(str.c_str(), str.length());
    return md.digest().str();
}

std::string krico::backup::md5_sum(const std::string &str) {
    const auto md = Digest::md5();
    md.update(str.c_str(), str.length());
    return md.digest().str();
}

std::string Digest::result::str() const {
    std::stringstream ss;
    for (int i = 0; i < len_; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(md_[i]);
    }
    return ss.str();
}

fs::path Digest::result::path(const uint8_t dirs) const {
    std::stringstream ss;
    for (int i = 0; i < len_; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(md_[i]);
        if (i < dirs) ss << fs::path::preferred_separator;
    }
    return ss.str();
}

void Digest::result::parse(result &r, const std::string &s) {
    if (s.length() > 2 * EVP_MAX_MD_SIZE) {
        THROW_EXCEPTION("Digest result to long (len="
            + std::to_string(s.length()) + " > max=" + std::to_string(2 * EVP_MAX_MD_SIZE) + ")" + " '" + s + "'");
    }
    if (s.length() % 2 != 0) {
        THROW_EXCEPTION("Digest result must be even (len=" + std::to_string(s.length()) + ")" + " '" + s + "'");
    }

    r.len_ = 0;
    const char *ptr = s.c_str();
    for (int i = 0; i < s.length(); i += 2) {
        const char *eptr = ptr + 2;
        if (auto [_, ec] = std::from_chars(ptr, eptr, r.md_[r.len_++], 16); ec != std::errc()) {
            THROW_ERROR_CODE("Failed to parse '" + s + "'", std::make_error_code(ec));
        }
        ptr = eptr;
    }
}
