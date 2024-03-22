#pragma once

#include <openssl/evp.h>
#include <string>
#include <__filesystem/filesystem_error.h>

//!
//! Digest routines...
//!

namespace krico::backup {
    namespace DigestLength {
        static constexpr unsigned int SHA1 = 20;
        static constexpr unsigned int SHA256 = 32;
        static constexpr unsigned int MD5 = 16;
    }

    class Digest {
    public:
        //!
        //! Construct a new Digest.
        //!
        //! It is more convenient to use one of the static methods such as Digest::sha256() and Digest::md5()
        //!
        explicit Digest(const std::string &algorithm, const std::string &properties = "");

        ~Digest();

        //!
        //! Create a new Digest with sha1 implementation
        //!
        [[nodiscard]] static Digest sha1();

        //!
        //! Create a new Digest with sha256 implementation
        //!
        [[nodiscard]] static Digest sha256();

        //!
        //! Create a new Digest with md5 implementation
        //!
        [[nodiscard]] static Digest md5();

        void reset() const;

        void update(const void *data, size_t len) const;


        struct result {
            uint8_t md_[EVP_MAX_MD_SIZE];
            unsigned int len_;

            [[nodiscard]] std::string str() const;

            //!
            //! @return the relative path for a file with this digest with `dirs` directories
            //!
            [[nodiscard]] std::filesystem::path path(uint8_t dirs) const;

            [[nodiscard]] constexpr bool is_zero() const {
                for (size_t i = 0; i < len_; ++i) {
                    if (md_[i] != 0) return false;
                }
                return true;
            }

            constexpr bool operator==(const result &rhs) const {
                if (len_ != rhs.len_) return false;
                for (size_t i = 0; i < len_; ++i) {
                    if (md_[i] != rhs.md_[i]) return false;
                }
                return true;
            }

            constexpr bool operator!=(const result &rhs) const {
                return !(*this == rhs);
            }
        };

        static constexpr result SHA1_ZERO{.len_ = DigestLength::SHA1};
        static constexpr result SHA256_ZERO{.len_ = DigestLength::SHA256};
        static constexpr result MD5_ZERO{.len_ = DigestLength::MD5};

        //!
        //! Compute the digest result
        //!
        [[nodiscard]] result digest() const;

        //!
        //! Compute the zero result (aka: result of digest-length wiht all zeroes)
        //!
        [[nodiscard]] result zero() const;

    private:
        EVP_MD *digest_{nullptr};
        EVP_MD_CTX *context_{nullptr};
    };

    //!
    //! Compute the sha1 checksum of str
    //!
    std::string sha1_sum(const std::string &str);

    //!
    //! Compute the sha256 checksum of str
    //!
    std::string sha256_sum(const std::string &str);

    //!
    //! Compute the md5 checksum of str
    //!
    std::string md5_sum(const std::string &str);
}
