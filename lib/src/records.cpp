#include "krico/backup/records.h"
#include "krico/backup/exception.h"
#include <spdlog/spdlog.h>

using namespace krico::backup::records;

namespace {
    class memmory_error final : public std::bad_alloc {
    public:
        explicit memmory_error(std::string what): what_(std::move(what)) {
        }

        [[nodiscard]] const char *what() const noexcept override { return what_.c_str(); }

    private:
        std::string what_;
    };
}

buffer::buffer() = default;

buffer::buffer(const size_t capacity) {
    void *mem = std::malloc(capacity);
    if (!mem) {
        THROW(memmory_error, "Failed to allocate " + std::to_string(capacity) + " bytes");
    }
    data_ = static_cast<uint8_t *>(mem);
    len_ = capacity;
}

buffer::~buffer() {
    if (data_) {
        std::free(data_);
        data_ = nullptr;
        len_ = 0;
    }
}

void buffer::resize(const size_t capacity) {
    void *mem = std::realloc(data_, capacity);
    if (!mem) {
        THROW(memmory_error, "Failed to reallocate " + std::to_string(capacity) + " bytes");
    }
    data_ = static_cast<uint8_t *>(mem);
    len_ = capacity;
}
