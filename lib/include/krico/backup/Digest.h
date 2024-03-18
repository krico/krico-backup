#pragma once

#include <openssl/evp.h>
#include <string>
#include <__filesystem/filesystem_error.h>

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
            //! @return the relative path for a file with this digest
            //!
            [[nodiscard]] std::filesystem::path path() const;

            bool operator==(const result &) const;
        };

        [[nodiscard]] result digest() const;

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
