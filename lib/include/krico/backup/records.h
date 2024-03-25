#pragma once

#include "krico/backup/uint8_utils.h"
#include "Digest.h"
#include "BackupDirectoryId.h"
#include <cstdint>
#include <string_view>
#include <cassert>
#include <filesystem>
#include <chrono>

namespace krico::backup::records {
    class buffer {
    public:
        buffer();

        explicit buffer(size_t capacity);

        ~buffer();

        buffer(const buffer &) = delete;

        void operator=(const buffer &) = delete;

        [[nodiscard]] const uint8_t *const_ptr(const size_t offset = 0) const {
            return data_ + offset;
        }

        [[nodiscard]] uint8_t *ptr(const size_t offset = 0) const {
            return data_ + offset;
        }

        [[nodiscard]] char *cptr(const size_t offset = 0) const {
            return reinterpret_cast<char *>(ptr(offset));
        }

        [[nodiscard]] const char *const_cptr(const size_t offset = 0) const {
            return reinterpret_cast<const char *>(const_ptr(offset));
        }

        void resize(size_t capacity);

    private:
        uint8_t *data_{nullptr};
        size_t len_{0};
    };

    template<typename T>
    struct codec;

    struct field_offset {
        virtual ~field_offset() = default;

        virtual void offset(size_t offset) = 0;

        [[nodiscard]] virtual size_t end_offset() const = 0;
    };

    template<typename T>
    class field final : public field_offset {
    public:
        explicit field(buffer &buffer): buffer_(buffer) {
        }

        void offset(const size_t offset) override {
            offset_ = offset;
        }

        T get() const {
            return codec<T>::get(buffer_.const_ptr(offset_));
        }

        void set(const T &v) {
            codec<T>::set(buffer_.ptr(offset_), v);
        }

        [[nodiscard]] size_t end_offset() const override {
            return offset_ + codec<T>::length(buffer_.ptr(offset_));
        }

    private:
        buffer &buffer_;
        size_t offset_{0};
    };

    template<>
    struct codec<uint8_t> {
        [[nodiscard]] static uint8_t get(const uint8_t *buf) {
            return *buf;
        }

        static void set(uint8_t *buf, const uint8_t v) {
            *buf = v;
        }

        [[nodiscard]] static size_t length(const uint8_t *) {
            return 1;
        }
    };

    template<>
    struct codec<uint16_t> {
        [[nodiscard]] static uint16_t get(const uint8_t *buf) {
            return get_le<uint16_t>(buf);
        }

        static void set(uint8_t *buf, const uint16_t v) {
            put_le(buf, v);
        }

        [[nodiscard]] static size_t length(const uint8_t *) {
            return 2;
        }
    };

    template<>
    struct codec<uint32_t> {
        [[nodiscard]] static uint32_t get(const uint8_t *buf) {
            return get_le<uint32_t>(buf);
        }

        static void set(uint8_t *buf, const uint32_t v) {
            put_le(buf, v);
        }

        [[nodiscard]] static size_t length(const uint8_t *) {
            return 4;
        }
    };

    template<>
    struct codec<uint64_t> {
        [[nodiscard]] static uint64_t get(const uint8_t *buf) {
            return get_le<uint64_t>(buf);
        }

        static void set(uint8_t *buf, const uint64_t v) {
            put_le(buf, v);
        }

        [[nodiscard]] static size_t length(const uint8_t *) {
            return 8;
        }
    };

    template<unsigned int DigestLength>
    struct digest_result : Digest::result {
        static constexpr unsigned int digest_length = DigestLength;

        digest_result(): result() {
        }

        explicit(false) digest_result(const result &r): result(r) /* NOLINT(*-explicit-constructor) */ {
        }
    };

    template<unsigned int DigestLength>
    struct codec<digest_result<DigestLength> > {
        static constexpr unsigned int digest_length = digest_result<DigestLength>::digest_length;

        [[nodiscard]] static digest_result<DigestLength> get(const uint8_t *buf) {
            Digest::result ret{.len_ = digest_length};
            std::memcpy(ret.md_, buf, digest_length);
            return ret;
        }

        static void set(uint8_t *buf, const digest_result<DigestLength> &v) {
            assert(v.len_ == digest_length);
            std::memcpy(buf, v.md_, digest_length);
        }

        [[nodiscard]] static size_t length(const uint8_t *) {
            return digest_length;
        }
    };

    //!
    //! Dynamic int is encoded to use the minimum possible bytes, namely:
    //!
    //! 1 byte (7 bits) `0???????` - from 0 - 127
    //! 2 bytes (14 bits) `10??????????????` - from 128 - 16383
    //! 4 bytes (30 bits) `11??????????????????????????????` - from 16384 - 1073741823
    //!
    struct dynamic_int {
        static constexpr uint8_t MAX1 = 0b01111111;
        static constexpr uint16_t MAX2 = 0b0011111111111111;
        static constexpr uint32_t MAX3 = 0b00111111111111111111111111111111;

        static constexpr uint8_t LENGTH1_MASK = 0b10000000;
        static constexpr uint8_t LENGTH2_MASK = 0b11000000;

        static constexpr uint16_t UINT16_MASK = 0b1000000000000000;
        static constexpr uint32_t UINT32_MASK = 0b11000000000000000000000000000000;

        uint32_t v;

        dynamic_int(): v() {
        }

        explicit(false) dynamic_int(const uint32_t &r): v(r) /* NOLINT(*-explicit-constructor) */ {
        }

        explicit(false) operator uint32_t() const /* NOLINT(*-explicit-constructor) */ { return v; }
    };

    template<>
    struct codec<dynamic_int> {
        [[nodiscard]] static dynamic_int get(const uint8_t *buf) {
            if ((*buf & dynamic_int::LENGTH2_MASK) == dynamic_int::LENGTH2_MASK) {
                const uint32_t u32 = get_be<uint32_t>(buf) & dynamic_int::MAX3;
                return u32;
            }
            if ((*buf & dynamic_int::LENGTH1_MASK) == dynamic_int::LENGTH1_MASK) {
                const uint16_t u16 = get_be<uint16_t>(buf) & static_cast<uint16_t>(dynamic_int::MAX2);
                return u16;
            }
            return *buf;
        }

        static void set(uint8_t *buf, const dynamic_int v) {
            if (v.v > dynamic_int::MAX2) {
                const uint32_t u32 = v.v | dynamic_int::UINT32_MASK;
                put_be<uint32_t>(buf, u32);
            } else if (v.v > dynamic_int::MAX1) {
                const uint16_t u16 = static_cast<uint16_t>(v.v) | dynamic_int::UINT16_MASK;
                put_be<uint16_t>(buf, u16);
            } else {
                const auto u8 = static_cast<uint8_t>(v.v);
                *buf = u8;
            }
        }

        [[nodiscard]] static size_t length(const uint8_t *buf) {
            if ((*buf & dynamic_int::LENGTH2_MASK) == dynamic_int::LENGTH2_MASK) {
                return 4;
            }
            if ((*buf & dynamic_int::LENGTH1_MASK) == dynamic_int::LENGTH1_MASK) {
                return 2;
            }
            return 1;
        }
    };

    template<>
    struct codec<std::string_view> {
        [[nodiscard]] static std::string_view get(const uint8_t *buf) {
            const auto len = codec<dynamic_int>::get(buf);
            const auto lenLen = codec<dynamic_int>::length(buf);
            return std::string_view{reinterpret_cast<const char *>(buf + lenLen), len};
        }

        static void set(uint8_t *buf, const std::string_view &v) {
            codec<dynamic_int>::set(buf, v.length());
            const auto lenLen = codec<dynamic_int>::length(buf);
            std::memcpy(buf + lenLen, v.data(), v.length());
        }

        [[nodiscard]] static size_t length(const uint8_t *buf) {
            const auto len = codec<dynamic_int>::get(buf);
            const auto lenLen = codec<dynamic_int>::length(buf);
            return lenLen + len;
        }
    };

    template<>
    struct codec<std::filesystem::path> {
        [[nodiscard]] static std::filesystem::path get(const uint8_t *buf) {
            return codec<std::string_view>::get(buf);
        }

        static void set(uint8_t *buf, const std::filesystem::path &v) {
            codec<std::string_view>::set(buf, v.string());
        }

        [[nodiscard]] static size_t length(const uint8_t *buf) {
            return codec<std::string_view>::length(buf);
        }
    };

    template<>
    struct codec<BackupDirectoryId> {
        [[nodiscard]] static BackupDirectoryId get(const uint8_t *buf) {
            return BackupDirectoryId{codec<std::string_view>::get(buf)};
        }

        static void set(uint8_t *buf, const BackupDirectoryId &v) {
            codec<std::string_view>::set(buf, v.str());
        }

        [[nodiscard]] static size_t length(const uint8_t *buf) {
            return codec<std::string_view>::length(buf);
        }
    };

    template<typename T> requires std::is_enum_v<T>
    struct codec<T> {
        static_assert(std::is_same_v<std::underlying_type_t<T>, uint8_t>, "only uint8_t enums supported");

        [[nodiscard]] static T get(const uint8_t *buf) {
            return static_cast<T>(*buf);
        }

        static void set(uint8_t *buf, const T v) {
            *buf = static_cast<uint8_t>(v);
        }

        [[nodiscard]] static size_t length(const uint8_t *) {
            return 1;
        }
    };

    template<>
    struct codec<std::chrono::year_month_day> {
        [[nodiscard]] static std::chrono::year_month_day get(const uint8_t *buf) {
            using namespace std::chrono;

            // YYYYMMDD
            auto date = codec<uint32_t>::get(buf);
            const auto d = date % 100;
            date /= 100;
            const auto m = date % 100;
            date /= 100;
            const int y = date; // NOLINT(*-narrowing-conversions)
            return year_month_day{year(y), month(m), day(d)};
        }

        static void set(uint8_t *buf, const std::chrono::year_month_day &v) {
            uint32_t date{0};
            if (v.ok()) {
                date = static_cast<int>(v.year()) * 10000 +
                       static_cast<unsigned>(v.month()) * 100 +
                       static_cast<unsigned>(v.day());
            }
            codec<uint32_t>::set(buf, date);
        }

        // ReSharper disable once CppDFAConstantFunctionResult
        [[nodiscard]] static size_t length(const uint8_t *buf) {
            return codec<uint32_t>::length(buf);
        }
    };

    template<>
    struct codec<std::chrono::system_clock::time_point> {
        [[nodiscard]] static std::chrono::system_clock::time_point get(const uint8_t *buf) {
            using namespace std::chrono;

            const auto nanos = codec<uint64_t>::get(buf);
            return system_clock::time_point{duration_cast<system_clock::duration>(nanoseconds(nanos))};
        }

        static void set(uint8_t *buf, const std::chrono::system_clock::time_point &v) {
            using namespace std::chrono;

            codec<uint64_t>::set(buf, duration_cast<nanoseconds>(v.time_since_epoch()).count());
        }

        // ReSharper disable once CppDFAConstantFunctionResult
        [[nodiscard]] static size_t length(const uint8_t *buf) {
            return codec<uint64_t>::length(buf);
        }
    };
}
