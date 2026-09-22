#include <cstddef>
#include <cstdint>
#include <ctex/io/image_io.hpp>
#include <span>

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
    const auto bytes = std::as_bytes(std::span(data, size));
    try {
        static_cast<void>(ctex::io::decode_image_memory({
            .bytes = bytes,
            .source_name = "fuzz.input",
            .limits = {.maximum_width = 4096,
                       .maximum_height = 4096,
                       .maximum_decoded_bytes = 8ULL << 20},
        }));
    } catch (const ctex::io::ImageIoError&) {
    }
    return 0;
}
