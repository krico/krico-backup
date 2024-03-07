#include "krico/backup/Digest.h"
#include "krico/backup/exception.h"
#include <openssl/err.h>
#include <string>
#include <sstream>
#include <iomanip>

using namespace krico::backup;

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

void Digest::update(const void *data, size_t len) const {
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

std::string Digest::result::str() const {
    std::stringstream ss;
    for (int i = 0; i < len_; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(md_[i]);
    }
    return ss.str();
}

bool Digest::result::operator==(const result &rhs) const {
    if (len_ != rhs.len_) return false;
    for (size_t i = 0; i < len_; ++i) {
        if (md_[i] != rhs.md_[i]) return false;
    }
    return true;
}