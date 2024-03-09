#pragma once

#include <filesystem>
#include <optional>

//!
//! Utility functions and macros to perform I/O operations.
//!
//! When using the macros, you must include dependencies yourself (so we can have multiple macros without all deps)
//!

//!
//! Create a directory and throw exception if it fails
//!
#define MKDIR(dir) do {                                                                                   \
  if (std::error_code error_code$; !std::filesystem::create_directory(dir, error_code$) || error_code$) { \
    if (error_code$)                                                                                      \
      THROW_ERROR_CODE("Failed to create directory '" + dir.string() + "'", error_code$);                 \
    THROW_EXCEPTION("Failed to create directory '" + dir.string() + "'");                                 \
  }                                                                                                       \
} while(false)


//!
//! Create a directory and its parents (does not fail if dir exists), throw exception if it fails
//!
//! @return true if create_directories created a dir
//!
#define MKDIRS(dir) ({                                                                  \
  std::error_code error_code$;                                                          \
  bool ret = std::filesystem::create_directories(dir, error_code$);                     \
  if (error_code$)                                                                      \
    THROW_ERROR_CODE("Failed to create directory '" + dir.string() + "'", error_code$); \
  ret;                                                                                  \
})

//!
//! Copies file from, to
//!
#define COPY_FILE(from, to) do {                                                                      \
  std::error_code error_code$;                                                                        \
  std::filesystem::copy_file(from, to, error_code$);                                                  \
  if (error_code$)                                                                                    \
    THROW_ERROR_CODE("Failed to copy '" + from.string() + "' -> '" + to.string() + "'", error_code$); \
} while (false)

//!
//! Rename a file from, to
//!
#define RENAME_FILE(from, to) do {                                                                      \
  std::error_code error_code$;                                                                          \
  std::filesystem::rename(from, to, error_code$);                                                       \
  if (error_code$)                                                                                      \
    THROW_ERROR_CODE("Failed to rename '" + from.string() + "' -> '" + to.string() + "'", error_code$); \
} while (false)

//!
//! Creates a hard linke target, link
//!
#define CREATE_HARD_LINK(target, link) do {                                                                           \
  std::error_code error_code$;                                                                                        \
  std::filesystem::create_hard_link(target, link, error_code$);                                                       \
  if (error_code$)                                                                                                    \
    THROW_ERROR_CODE("Failed to create hard link '" + target.string() + "' -> '" + link.string() + "'", error_code$); \
} while (false)

//!
//! Creates a hard link target, link
//!
//! @return target of the symbolic link
//!
#define READ_SYMLINK(file) ({                                                        \
  std::error_code error_code$;                                                       \
  const auto target = std::filesystem::read_symlink(file, error_code$);              \
  if (error_code$)                                                                   \
    THROW_ERROR_CODE("Failed to read symlink '" + file.string() + "'", error_code$); \
  target;                                                                            \
})


//!
//! Creates a hard linke target, link
//!
#define CREATE_SYMLINK(target, link) do {                                                                           \
  std::error_code error_code$;                                                                                      \
  std::filesystem::create_symlink(target, link, error_code$);                                                       \
  if (error_code$)                                                                                                  \
    THROW_ERROR_CODE("Failed to create symlink '" + target.string() + "' -> '" + link.string() + "'", error_code$); \
} while (false)

namespace krico::backup {
    //!
    //! @return true if path is a lexical sub-path of base (e.g. /a/b is sub_path of /a and /a/b is NOT of /b)
    //!
    bool is_lexical_sub_path(const std::filesystem::path &path, const std::filesystem::path &base);

    std::filesystem::path lexically_relative_symlink_target(const std::filesystem::path &link,
                                                            const std::filesystem::path &target,
                                                            const std::filesystem::path &base);
}
