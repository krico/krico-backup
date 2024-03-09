#pragma once

#include <filesystem>
#include <iterator>

namespace krico::backup {
    class Directory; // fwd-decl
    class File; // fwd-decl
    class Symlink; // fwd-decl

    //!
    //! An entry such as a File or a Directory obtained by either creating a new Directory or navigating from
    //! a created one.
    //!
    struct directory_entry {
        using ptr = std::unique_ptr<directory_entry>;

        directory_entry(const std::filesystem::path &base, const std::filesystem::path &path);

        virtual ~directory_entry() = default;

        //!
        //! @return the relative path to this directory_entry from initial Directory used to navigate to it.
        //!
        [[nodiscard]] const std::filesystem::path &relative_path() const;

        //!
        //! @return the absolute path to this directory_entry.
        //!
        [[nodiscard]] std::filesystem::path absolute_path() const;

        [[nodiscard]] virtual bool is_file() const { return false; };

        [[nodiscard]] virtual const File &as_file() const;

        [[nodiscard]] virtual bool is_directory() const { return false; };

        [[nodiscard]] virtual const Directory &as_directory() const;

        [[nodiscard]] virtual bool is_symlink() const { return false; };

        [[nodiscard]] virtual const Symlink &as_symlink() const;

    protected:
        std::filesystem::path basePath_;
        std::filesystem::path absolutePath_;
        std::filesystem::path relativePath_;
    };

    class Directory final : public directory_entry {
    public:
        class iterator {
        public:
            using iterator_category = std::input_iterator_tag;
            using value_type = directory_entry;
            using difference_type = std::ptrdiff_t;
            using pointer = value_type *;
            using reference = value_type &;

            ~iterator();

            iterator(const iterator &rhs);

            iterator &operator=(const iterator &rhs);

            iterator &operator++() {
                ++it_;
                return *this;
            }

            iterator operator++(int) {
                const iterator retval = *this;
                ++(*this);
                return retval;
            }

            bool operator==(const iterator &other) const { return it_ == other.it_ && directory_ == other.directory_; }
            bool operator!=(const iterator &other) const { return !(*this == other); }

            reference operator*();

        private:
            const Directory *directory_{nullptr};
            std::filesystem::directory_iterator it_;
            directory_entry *current_{nullptr};

            explicit iterator(const Directory &directory, std::filesystem::directory_iterator it);

            friend class Directory;
        };

        explicit Directory(const std::filesystem::path &dir);

        Directory(const Directory &parent, const std::filesystem::path &dir);

        [[nodiscard]] bool is_directory() const override { return true; }

        [[nodiscard]] const Directory &as_directory() const override { return *this; }

        [[nodiscard]] iterator begin() const;

        [[nodiscard]] iterator end() const;

        friend class File;
        friend class Symlink;
    };

    class File final : public directory_entry {
    public:
        explicit File(const Directory &parent, const std::filesystem::path &file);

        [[nodiscard]] bool is_file() const override { return true; }

        [[nodiscard]] const File &as_file() const override { return *this; }
    };

    class Symlink final : public directory_entry {
    public:
        Symlink(const Directory &parent, const std::filesystem::path &file);

        Symlink(const Directory &parent, const std::filesystem::path &file, bool tagetIsDir);

        [[nodiscard]] const std::filesystem::path &target() const { return target_; }

        [[nodiscard]] const std::filesystem::path &relative_target() const { return relativeTarget_; }

        [[nodiscard]] bool is_target_dir() const { return targetIsDir_; }

        [[nodiscard]] bool is_symlink() const override { return true; }

        [[nodiscard]] const Symlink &as_symlink() const override { return *this; }

    private:
        std::filesystem::path target_;
        bool targetIsDir_;
        std::filesystem::path relativeTarget_;
    };
}
