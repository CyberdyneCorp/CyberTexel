#include <cstddef>
#include <cstdint>
#include <ctex/io/project_container.hpp>
#include <span>

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
    const auto bytes = std::as_bytes(std::span(data, size));
    ctex::io::ProjectContainerReadLimits limits;
    limits.maximum_input_bytes = 1ULL << 20;
    limits.maximum_total_allocation_bytes = 8ULL << 20;
    limits.maximum_sections = 4096;
    limits.maximum_images = 4096;
    limits.maximum_tiles = 4096;
    limits.maximum_resources = 4096;
    limits.maximum_assets = 4096;
    limits.maximum_asset_dependencies = 4096;
    limits.maximum_string_bytes = 1ULL << 16;
    limits.maximum_decoded_tile_bytes = 4ULL << 20;
    limits.maximum_packed_resource_bytes = 4ULL << 20;
    limits.maximum_asset_payload_bytes = 4ULL << 20;
    try {
        static_cast<void>(ctex::io::probe_project_container_version(bytes));
    } catch (const ctex::io::ProjectContainerError&) {
    }
    try {
        static_cast<void>(ctex::io::read_project_container(bytes, limits));
    } catch (const ctex::io::ProjectContainerError&) {
    }
    return 0;
}
