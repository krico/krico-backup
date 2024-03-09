#include "krico/backup/Directory.h"
#include "krico/backup/exception.h"
#include "krico/backup/io.h"
#include <spdlog/spdlog.h>
#include <utility>

using namespace krico::backup;
namespace fs = std::filesystem;

directory_entry::directory_entry(const std::filesystem::path &base, const std::filesystem::path &path)
    : basePath_(absolute(base)), absolutePath_(absolute(path)),
      relativePath_(absolutePath_.lexically_proximate(basePath_)) {
}

std::filesystem::path directory_entry::absolute_path() const {
    return absolutePath_;
}

const File &directory_entry::as_file() const {
    THROW_NOT_IMPLEMENTED("as_file()");
}

const Directory &directory_entry::as_directory() const {
    THROW_NOT_IMPLEMENTED("as_directory()");
}

const Symlink &directory_entry::as_symlink() const {
    THROW_NOT_IMPLEMENTED("as_symlink()");
}

Directory::iterator::~iterator() {
    delete current_;
    current_ = nullptr;
}

Directory::iterator::iterator(const iterator &rhs) : directory_(rhs.directory_), it_(rhs.it_) {
}

Directory::iterator &Directory::iterator::operator=(const iterator &rhs) {
    if (this == &rhs) return *this;
    directory_ = rhs.directory_;
    it_ = rhs.it_;
    delete current_;
    current_ = rhs.current_;
    return *this;
}

const std::filesystem::path &directory_entry::relative_path() const {
    return relativePath_;
}

Directory::iterator::reference Directory::iterator::operator*() {
    const auto &entry = *it_;
    if (current_ && current_->absolute_path() == entry.path()) {
        return *current_;
    }

    delete current_;
    if (entry.is_symlink()) {
        current_ = new Symlink(*directory_, entry.path(), entry.is_directory());
    } else if (entry.is_regular_file()) {
        current_ = new File(*directory_, entry.path());
    } else if (entry.is_directory()) {
        current_ = new Directory(*directory_, entry.path());
    } else {
        // TODO: handle this
        THROW_EXCEPTION("Entry is neither a file nor a directory");
    }
    return *current_;
}

Directory::iterator::iterator(const Directory &directory, std::filesystem::directory_iterator it)
    : directory_(&directory), it_(std::move(it)) {
}

Directory::Directory(const fs::path &dir) : directory_entry(dir, dir) {
}

Directory::Directory(const Directory &parent, const std::filesystem::path &dir)
    : directory_entry(parent.basePath_, dir) {
}

Directory::iterator Directory::begin() const {
    return iterator(*this, fs::directory_iterator(absolutePath_));
}

Directory::iterator Directory::end() const {
    return iterator(*this, fs::directory_iterator());
}

File::File(const Directory &parent, const std::filesystem::path &file)
    : directory_entry(parent.basePath_, file) {
}

Symlink::Symlink(const Directory &parent, const std::filesystem::path &file)
    : directory_entry(parent.basePath_, file) {
    target_ = READ_SYMLINK(absolutePath_);
    relativeTarget_ = lexically_relative_symlink_target(absolutePath_, target_, basePath_);
    targetIsDir_ = fs::is_directory(canonical(file));
}

Symlink::Symlink(const Directory &parent, const std::filesystem::path &file, const bool tagetIsDir)
    : directory_entry(parent.basePath_, file) {
    target_ = READ_SYMLINK(absolutePath_);
    relativeTarget_ = lexically_relative_symlink_target(absolutePath_, target_, basePath_);
    targetIsDir_ = tagetIsDir;
}
