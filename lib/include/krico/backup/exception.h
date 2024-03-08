#pragma once

#include <stdexcept>
#include <system_error>
#include <sstream>

#define THROW(exc, ...) do { \
  spdlog::error("{}:{} throw {}({})", __FILE__, __LINE__, #exc, #__VA_ARGS__);    \
  throw exc(__VA_ARGS__);    \
} while(false)

#define THROW_EXCEPTION(reason) \
  THROW(::krico::backup::exception, reason)

#define THROW_ERROR_CODE(reason, ec) \
  THROW(::krico::backup::errno_exception, reason, ec)

#define THROW_ERRNO(reason) \
  THROW(::krico::backup::errno_exception, reason, errno)

namespace krico::backup {
    struct exception : std::runtime_error {
        explicit exception(const char *str): std::runtime_error(str) {
        }

        explicit exception(const std::string &str): std::runtime_error(str) {
        }

        explicit exception(const std::runtime_error &e): std::runtime_error(e) {
        }
    };

    struct errno_exception final : exception {
        explicit errno_exception(const std::error_code &error_code): exception(error_code.message()) {
        }

        explicit errno_exception(const int errno_value)
            : errno_exception(std::make_error_code(static_cast<std::errc>(errno_value))) {
        }

        errno_exception(const std::string &msg, const std::error_code &error_code)
            : exception(msg + " (" + error_code.message() + ")") {
        }

        errno_exception(const std::string &msg, const int errno_value)
            : errno_exception(msg, std::make_error_code(static_cast<std::errc>(errno_value))) {
        }
    };
}
