#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdlib>
#include <ctex/io/project_container.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <span>
#include <string_view>
#include <vector>

namespace {

using namespace ctex;
using namespace ctex::io;

bool expect(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

std::uint64_t read_u64_le(std::span<const std::byte> bytes, std::size_t offset) {
    std::uint64_t value = 0;
    for (unsigned index = 0; index < 8; ++index) {
        value |= static_cast<std::uint64_t>(std::to_integer<std::uint8_t>(bytes[offset + index]))
                 << (index * 8U);
    }
    return value;
}

template <typename Callable>
bool expect_error(Callable&& callable, ProjectContainerErrorCode code, std::string_view message) {
    try {
        callable();
    } catch (const ProjectContainerError& error) {
        return expect(error.code() == code, "project container returned the wrong error code");
    } catch (...) {
    }
    return expect(false, message);
}

bool schema_version_is_probeable_without_the_body() {
    const std::vector<std::byte> encoded = write_project_container({});
    constexpr std::size_t header_bytes = 40;
    const ContainerSchemaVersion full = probe_project_container_version(encoded);
    const ContainerSchemaVersion header_only =
        probe_project_container_version(std::span(encoded).first(header_bytes));
    const bool truncated = expect_error(
        [&] {
            static_cast<void>(
                probe_project_container_version(std::span(encoded).first(header_bytes - 1)));
        },
        ProjectContainerErrorCode::malformed_header,
        "truncated project container header was accepted");
    return expect(full == current_container_schema && header_only == current_container_schema &&
                      full.major == container_writer_version.major &&
                      full.minor == container_writer_version.minor &&
                      full.patch == container_writer_version.patch,
                  "container schema version is not shared with or probeable from the header") &&
           truncated;
}

bool sparse_tiled_pixels_round_trip_losslessly() {
    constexpr std::uint32_t canvas_size = 16'384;
    const std::array clear{std::byte{7}};
    image::TiledImage source(canvas_size, canvas_size,
                             {.channel_type = image::ChannelType::uint8_unorm, .channel_count = 1},
                             image::default_tile_size, clear);
    const std::array painted{std::byte{231}};
    source.write_pixel(canvas_size - 1, canvas_size - 1, painted);

    const StoredTiledImage stored = snapshot_tiled_image("layer/base-color", source);
    ProjectContainer container;
    container.tiled_images.push_back(stored);
    const std::vector<std::byte> encoded = write_project_container(container);
    const ProjectContainerReadResult decoded = read_project_container(encoded);
    const image::TiledImage restored = restore_tiled_image(decoded.container.tiled_images.front());

    return expect(stored.occupied_tiles.size() == 1 &&
                      stored.occupied_tiles.front().coordinate == image::TileCoordinate{255, 255},
                  "sparse image snapshot stored unoccupied tiles") &&
           expect(encoded.size() < 1ULL << 20,
                  "one occupied tile scaled storage with the full 16384 canvas") &&
           expect(decoded.container.tiled_images == std::vector<StoredTiledImage>{stored} &&
                      decoded.report.unknown_parts.empty(),
                  "compressed per-tile container round trip changed tiled pixels") &&
           expect(restored.width() == canvas_size && restored.height() == canvas_size &&
                      restored.read_pixel(0, 0)[0] == clear[0] &&
                      restored.read_pixel(canvas_size - 1, canvas_size - 1)[0] == painted[0] &&
                      restored.resident_pixel_bytes() == source.resident_pixel_bytes(),
                  "restored sparse image changed metadata, clear pixels, or painted pixels");
}

bool partial_edge_tiles_store_only_their_extent() {
    const std::array clear{std::byte{0}, std::byte{0}, std::byte{0}, std::byte{0}};
    const std::array painted{std::byte{1}, std::byte{2}, std::byte{3}, std::byte{4}};
    image::TiledImage source(
        65, 66, {.channel_type = image::ChannelType::uint16_unorm, .channel_count = 2}, 64, clear);
    source.write_pixel(64, 65, painted);
    const StoredTiledImage stored = snapshot_tiled_image("layer/edge", source);
    ProjectContainer container;
    container.tiled_images.push_back(stored);
    const ProjectContainerReadResult decoded =
        read_project_container(write_project_container(container));
    const image::TiledImage restored = restore_tiled_image(decoded.container.tiled_images.front());

    return expect(stored.occupied_tiles.size() == 1 &&
                      stored.occupied_tiles.front().coordinate == image::TileCoordinate{1, 1} &&
                      stored.occupied_tiles.front().extent == image::TileExtent{1, 2} &&
                      stored.occupied_tiles.front().pixels.size() == 8,
                  "partial edge tile retained padded rows or columns") &&
           expect(std::equal(painted.begin(), painted.end(), restored.read_pixel(64, 65).begin()),
                  "partial edge tile did not round-trip its multi-byte pixel");
}

bool unknown_newer_sections_survive_open_and_resave() {
    ProjectContainer future;
    future.schema_version.patch += 1;
    future.opaque_sections.push_back(
        {.kind = 77,
         .version = 9,
         .payload = {std::byte{0xde}, std::byte{0xad}, std::byte{0xbe}, std::byte{0xef}}});
    const ProjectContainerReadResult opened =
        read_project_container(write_project_container(future));
    const ProjectContainerReadResult reopened =
        read_project_container(write_project_container(opened.container));

    return expect(opened.report.newer_schema && opened.report.unknown_parts.size() == 1 &&
                      opened.report.unknown_parts.front().section_kind == 77 &&
                      opened.container.opaque_sections == future.opaque_sections,
                  "newer unknown container section was not reported and preserved") &&
           expect(reopened.container.schema_version == future.schema_version &&
                      reopened.container.opaque_sections == future.opaque_sections &&
                      reopened.report.unknown_parts.size() == 1,
                  "resaving an older-reader result changed its unknown section");
}

bool unknown_tile_encoding_is_preserved_as_one_opaque_section() {
    const std::array clear{std::byte{0}};
    const std::array painted{std::byte{1}};
    image::TiledImage source(
        1, 1, {.channel_type = image::ChannelType::uint8_unorm, .channel_count = 1}, 1, clear);
    source.write_pixel(0, 0, painted);
    ProjectContainer original;
    original.tiled_images.push_back(snapshot_tiled_image("future", source));
    std::vector<std::byte> encoded = write_project_container(original);

    constexpr std::size_t header_and_section_headers = 40 + 16;
    const std::size_t tile_payload_size = static_cast<std::size_t>(read_u64_le(encoded, 40 + 8));
    const std::size_t compression_offset =
        header_and_section_headers + 4 + 4 + 6 + 12 + 1 + 1 + 2 + 1 + 4 + 16;
    encoded.at(compression_offset) = std::byte{127};
    const std::vector<std::byte> original_payload(
        encoded.begin() + header_and_section_headers,
        encoded.begin() + header_and_section_headers + tile_payload_size);
    const ProjectContainerReadResult opened = read_project_container(encoded);
    const ProjectContainerReadResult reopened =
        read_project_container(write_project_container(opened.container));

    return expect(opened.container.tiled_images.empty() &&
                      opened.container.opaque_sections.size() == 1 &&
                      opened.container.opaque_sections.front().kind == tiled_pixel_section_kind &&
                      opened.container.opaque_sections.front().payload == original_payload &&
                      opened.report.unknown_parts.size() == 1,
                  "unknown tile compression was decoded, dropped, or changed") &&
           expect(reopened.container.opaque_sections == opened.container.opaque_sections &&
                      reopened.report.unknown_parts.size() == 1,
                  "opaque current-version tile section could not be resaved");
}

bool invalid_tile_metadata_is_refused_before_writing() {
    StoredTiledImage invalid{
        .resource_id = "layer/invalid",
        .width = 64,
        .height = 64,
        .tile_size = 64,
        .format = {.channel_type = image::ChannelType::uint8_unorm, .channel_count = 1},
        .clear_pixel = {std::byte{0}},
        .occupied_tiles = {{.coordinate = {0, 0},
                            .extent = {64, 64},
                            .compression = TileCompression::zlib_deflate,
                            .pixels = {std::byte{1}}}},
    };
    ProjectContainer container;
    container.tiled_images.push_back(std::move(invalid));
    return expect_error([&] { static_cast<void>(write_project_container(container)); },
                        ProjectContainerErrorCode::invalid_tile,
                        "container writer accepted a tile whose payload did not match its extent");
}

bool resources_round_trip_and_resolve_without_blocking_open() {
    const std::filesystem::path directory =
        std::filesystem::temp_directory_path() / "ctex-project-container-resource-test";
    std::error_code filesystem_error;
    std::filesystem::remove_all(directory, filesystem_error);
    std::filesystem::create_directories(directory / "images", filesystem_error);
    if (!expect(!filesystem_error, "could not create project resource test directory")) {
        return false;
    }
    const std::array referenced_bytes{std::byte{0x89}, std::byte{'P'}, std::byte{'N'},
                                      std::byte{'G'}};
    {
        std::ofstream stream(directory / "images/albedo.png", std::ios::binary);
        stream.write(reinterpret_cast<const char*>(referenced_bytes.data()),
                     static_cast<std::streamsize>(referenced_bytes.size()));
    }

    ProjectContainer container;
    container.resources = {
        {.identifier = "texture/albedo",
         .kind = "image",
         .relative_path = "images/albedo.png",
         .packed_bytes = std::nullopt},
        {.identifier = "font/labels",
         .kind = "font",
         .relative_path = "fonts/labels.woff2",
         .packed_bytes = std::vector<std::byte>{std::byte{'f'}, std::byte{'o'}, std::byte{'n'},
                                                std::byte{'t'}}},
        {.identifier = "maps/ambient-occlusion",
         .kind = "mesh-map",
         .relative_path = "maps/ao.png",
         .packed_bytes = std::nullopt},
        {.identifier = "mesh/source",
         .kind = "mesh",
         .relative_path = "meshes/source.glb",
         .packed_bytes = std::vector<std::byte>{std::byte{'g'}, std::byte{'l'}, std::byte{'b'}}},
    };
    const ProjectContainerReadResult opened =
        read_project_container(write_project_container(container));
    const ProjectResourceResolution resolution =
        resolve_project_resources(opened.container, directory);
    std::filesystem::remove_all(directory, filesystem_error);

    return expect(opened.container.resources == container.resources &&
                      opened.report.unknown_parts.empty(),
                  "project resources did not round-trip losslessly") &&
           expect(resolution.resources.size() == 4 && !resolution.complete() &&
                      resolution.missing_identifiers ==
                          std::vector<std::string>{"maps/ambient-occlusion"},
                  "missing referenced project resource was not reported by identity") &&
           expect(resolution.resources[0].status == ProjectResourceStatus::referenced &&
                      resolution.resources[0].bytes ==
                          std::vector<std::byte>(referenced_bytes.begin(), referenced_bytes.end()),
                  "referenced project resource did not resolve relative to the project") &&
           expect(resolution.resources[1].status == ProjectResourceStatus::packed &&
                      resolution.resources[1].bytes == *container.resources[1].packed_bytes &&
                      resolution.resources[2].status == ProjectResourceStatus::missing &&
                      resolution.resources[3].status == ProjectResourceStatus::packed,
                  "packed or missing project resource status is incorrect");
}

bool unsafe_or_duplicate_resource_identities_are_refused() {
    ProjectContainer unsafe;
    unsafe.resources.push_back({.identifier = "unsafe",
                                .kind = "image",
                                .relative_path = "../outside.png",
                                .packed_bytes = std::nullopt});
    const bool unsafe_refused =
        expect_error([&] { static_cast<void>(write_project_container(unsafe)); },
                     ProjectContainerErrorCode::invalid_resource,
                     "container writer accepted a resource path outside the project directory");

    ProjectContainer duplicate;
    duplicate.resources = {
        {.identifier = "same",
         .kind = "image",
         .relative_path = "first.png",
         .packed_bytes = std::nullopt},
        {.identifier = "same",
         .kind = "font",
         .relative_path = "second.woff2",
         .packed_bytes = std::nullopt},
    };
    const bool duplicate_refused =
        expect_error([&] { static_cast<void>(write_project_container(duplicate)); },
                     ProjectContainerErrorCode::invalid_resource,
                     "container writer accepted duplicate resource identities");
    return unsafe_refused && duplicate_refused;
}

bool unknown_resource_storage_is_preserved() {
    ProjectContainer original;
    original.resources.push_back({.identifier = "future",
                                  .kind = "image",
                                  .relative_path = "future.bin",
                                  .packed_bytes = std::nullopt});
    std::vector<std::byte> encoded = write_project_container(original);
    constexpr std::size_t resource_section_offset = 40 + 16 + 4;
    constexpr std::size_t resource_payload_offset = resource_section_offset + 16;
    constexpr std::size_t storage_offset = resource_payload_offset + 4 + 4 + 6 + 4 + 5 + 4 + 10;
    encoded.at(storage_offset) = std::byte{127};
    const std::size_t resource_payload_size =
        static_cast<std::size_t>(read_u64_le(encoded, resource_section_offset + 8));
    const std::vector<std::byte> original_payload(
        encoded.begin() + resource_payload_offset,
        encoded.begin() + resource_payload_offset + resource_payload_size);
    const ProjectContainerReadResult opened = read_project_container(encoded);
    const ProjectContainerReadResult reopened =
        read_project_container(write_project_container(opened.container));

    return expect(
               opened.container.resources.empty() && opened.container.opaque_sections.size() == 1 &&
                   opened.container.opaque_sections.front().kind == project_resource_section_kind &&
                   opened.container.opaque_sections.front().payload == original_payload &&
                   opened.report.unknown_parts.size() == 1,
               "unknown resource storage was decoded, dropped, or changed") &&
           expect(reopened.container.opaque_sections == opened.container.opaque_sections &&
                      reopened.report.unknown_parts.size() == 1,
                  "opaque current-version resource section could not be resaved");
}

bool write_determinism_artifact() {
    const char* output_directory = std::getenv("CTEX_DETERMINISM_OUTPUT_DIR");
    if (output_directory == nullptr) {
        return expect(false, "determinism output directory is not set");
    }
    const std::array clear{std::byte{0}, std::byte{0}, std::byte{0}, std::byte{0}};
    image::TiledImage image(
        130, 65, {.channel_type = image::ChannelType::uint16_unorm, .channel_count = 2}, 64, clear);
    const std::array first{std::byte{0x34}, std::byte{0x12}, std::byte{0xcd}, std::byte{0xab}};
    const std::array second{std::byte{0xff}, std::byte{0x00}, std::byte{0x80}, std::byte{0x00}};
    image.write_pixel(0, 0, first);
    image.write_pixel(129, 64, second);
    ProjectContainer container;
    container.tiled_images.push_back(snapshot_tiled_image("layers/paint/pbr.base-color", image));
    container.resources.push_back(
        {.identifier = "mesh/source",
         .kind = "mesh",
         .relative_path = "meshes/source.glb",
         .packed_bytes = std::vector<std::byte>{std::byte{'g'}, std::byte{'l'}, std::byte{'b'}}});
    container.opaque_sections.push_back(
        {.kind = 0x80000001U,
         .version = 3,
         .payload = {std::byte{'f'}, std::byte{'u'}, std::byte{'t'}, std::byte{'u'}, std::byte{'r'},
                     std::byte{'e'}}});
    const std::vector<std::byte> encoded = write_project_container(container);
    const std::filesystem::path path =
        std::filesystem::path(output_directory) / "project-container.ctex";
    std::ofstream stream(path, std::ios::binary);
    stream.write(reinterpret_cast<const char*>(encoded.data()),
                 static_cast<std::streamsize>(encoded.size()));
    return expect(stream.good(), "could not write project-container determinism artifact");
}

}  // namespace

int main(int argc, char** argv) {
    if (argc == 2 && std::string_view(argv[1]) == "--write-determinism") {
        return write_determinism_artifact() ? 0 : 1;
    }
    return schema_version_is_probeable_without_the_body() &&
                   sparse_tiled_pixels_round_trip_losslessly() &&
                   partial_edge_tiles_store_only_their_extent() &&
                   unknown_newer_sections_survive_open_and_resave() &&
                   unknown_tile_encoding_is_preserved_as_one_opaque_section() &&
                   invalid_tile_metadata_is_refused_before_writing() &&
                   resources_round_trip_and_resolve_without_blocking_open() &&
                   unsafe_or_duplicate_resource_identities_are_refused() &&
                   unknown_resource_storage_is_preserved()
               ? 0
               : 1;
}
