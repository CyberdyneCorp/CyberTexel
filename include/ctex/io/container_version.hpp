#ifndef CTEX_IO_CONTAINER_VERSION_HPP
#define CTEX_IO_CONTAINER_VERSION_HPP

#include <ctex/version.h>

#include <cstdint>
#include <string_view>

namespace ctex::io {

struct ContainerWriterVersion {
    std::uint32_t major;
    std::uint32_t minor;
    std::uint32_t patch;
    std::string_view string;
};

inline constexpr ContainerWriterVersion container_writer_version{
    CTEX_VERSION_MAJOR,
    CTEX_VERSION_MINOR,
    CTEX_VERSION_PATCH,
    CTEX_VERSION_STRING,
};

}  // namespace ctex::io

#endif
