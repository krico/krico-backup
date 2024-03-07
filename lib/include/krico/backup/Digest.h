#pragma once

#include <openssl/evp.h>
#include <string>

//!
//! Digest routines...
//!

namespace krico::backup {
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
        //! Create a new Digest with sha256 implementation
        //!
        static Digest sha256();

        //!
        //! Create a new Digest with md5 implementation
        //!
        static Digest md5();

        void reset() const;

        void update(const void *data, size_t len) const;


        struct result {
            uint8_t md_[EVP_MAX_MD_SIZE];
            unsigned int len_;

            [[nodiscard]] std::string str() const;

            bool operator==(const result &) const;
        };

        result digest() const;

    private:
        EVP_MD *digest_{nullptr};
        EVP_MD_CTX *context_{nullptr};
    };
}
