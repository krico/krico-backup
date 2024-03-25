#pragma once

#include <cstdint>
#include <bit>

namespace krico::backup {
    template<typename N>
    N get_le(const uint8_t *ptr);

    template<typename N>
    void put_le(uint8_t *ptr, N v);

    template<typename N>
    N get_be(const uint8_t *ptr);

    template<typename N>
    void put_be(uint8_t *ptr, N v);

    template<>
    inline uint16_t get_le(const uint8_t *ptr) {
        auto ts = *static_cast<const uint16_t *>(static_cast<const void *>(ptr));
        if constexpr (std::endian::native == std::endian::little) {
            return ts;
        } else {
            // ReSharper disable once CppDFAUnreachableCode
            return __builtin_bswap16(ts);
        }
    }

    template<>
    inline void put_le(uint8_t *ptr, const uint16_t v) {
        if constexpr (std::endian::native == std::endian::little) {
            *static_cast<uint16_t *>(static_cast<void *>(ptr)) = v;
        } else {
            // ReSharper disable once CppDFAUnreachableCode
            *static_cast<uint16_t *>(static_cast<void *>(ptr)) = __builtin_bswap16(v);
        }
    }

    template<>
    inline uint16_t get_be(const uint8_t *ptr) {
        auto ts = *static_cast<const uint16_t *>(static_cast<const void *>(ptr));
        if constexpr (std::endian::native != std::endian::little) {
            return ts;
        } else {
            // ReSharper disable once CppDFAUnreachableCode
            return __builtin_bswap16(ts);
        }
    }

    template<>
    inline void put_be(uint8_t *ptr, const uint16_t v) {
        if constexpr (std::endian::native != std::endian::little) {
            *static_cast<uint16_t *>(static_cast<void *>(ptr)) = v;
        } else {
            // ReSharper disable once CppDFAUnreachableCode
            *static_cast<uint16_t *>(static_cast<void *>(ptr)) = __builtin_bswap16(v);
        }
    }

    template<>
    inline uint32_t get_le(const uint8_t *ptr) {
        auto ts = *static_cast<const uint32_t *>(static_cast<const void *>(ptr));
        if constexpr (std::endian::native == std::endian::little) {
            return ts;
        } else {
            // ReSharper disable once CppDFAUnreachableCode
            return __builtin_bswap32(ts);
        }
    }

    template<>
    inline void put_le(uint8_t *ptr, const uint32_t v) {
        if constexpr (std::endian::native == std::endian::little) {
            *static_cast<uint32_t *>(static_cast<void *>(ptr)) = v;
        } else {
            // ReSharper disable once CppDFAUnreachableCode
            *static_cast<uint32_t *>(static_cast<void *>(ptr)) = __builtin_bswap32(v);
        }
    }

    template<>
    inline uint32_t get_be(const uint8_t *ptr) {
        auto ts = *static_cast<const uint32_t *>(static_cast<const void *>(ptr));
        if constexpr (std::endian::native != std::endian::little) {
            return ts;
        } else {
            // ReSharper disable once CppDFAUnreachableCode
            return __builtin_bswap32(ts);
        }
    }

    template<>
    inline void put_be(uint8_t *ptr, const uint32_t v) {
        if constexpr (std::endian::native != std::endian::little) {
            *static_cast<uint32_t *>(static_cast<void *>(ptr)) = v;
        } else {
            // ReSharper disable once CppDFAUnreachableCode
            *static_cast<uint32_t *>(static_cast<void *>(ptr)) = __builtin_bswap32(v);
        }
    }

    template<>
    inline uint64_t get_le(const uint8_t *ptr) {
        auto ts = *static_cast<const uint64_t *>(static_cast<const void *>(ptr));
        if constexpr (std::endian::native == std::endian::little) {
            return ts;
        } else {
            // ReSharper disable once CppDFAUnreachableCode
            return __builtin_bswap64(ts);
        }
    }

    template<>
    inline void put_le(uint8_t *ptr, const uint64_t v) {
        if constexpr (std::endian::native == std::endian::little) {
            *static_cast<uint64_t *>(static_cast<void *>(ptr)) = v;
        } else {
            // ReSharper disable once CppDFAUnreachableCode
            *static_cast<uint64_t *>(static_cast<void *>(ptr)) = __builtin_bswap64(v);
        }
    }

    template<>
    inline uint64_t get_be(const uint8_t *ptr) {
        auto ts = *static_cast<const uint64_t *>(static_cast<const void *>(ptr));
        if constexpr (std::endian::native != std::endian::little) {
            return ts;
        } else {
            // ReSharper disable once CppDFAUnreachableCode
            return __builtin_bswap64(ts);
        }
    }

    template<>
    inline void put_be(uint8_t *ptr, const uint64_t v) {
        if constexpr (std::endian::native != std::endian::little) {
            *static_cast<uint64_t *>(static_cast<void *>(ptr)) = v;
        } else {
            // ReSharper disable once CppDFAUnreachableCode
            *static_cast<uint64_t *>(static_cast<void *>(ptr)) = __builtin_bswap64(v);
        }
    }
}
