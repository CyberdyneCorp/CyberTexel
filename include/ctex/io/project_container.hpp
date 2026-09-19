#ifndef CTEX_IO_PROJECT_CONTAINER_HPP
#define CTEX_IO_PROJECT_CONTAINER_HPP

#include <cstddef>
#include <cstdint>
#include <ctex/image/tiled_image.hpp>
#include <ctex/io/container_version.hpp>
#include <filesystem>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

namespace ctex::io {

inline constexpr std::uint32_t tiled_pixel_section_kind = 1;
inline constexpr std::uint32_t tiled_pixel_section_version = 1;
inline constexpr std::uint32_t project_resource_section_kind = 2;
inline constexpr std::uint32_t project_resource_section_version = 1;

enum class TileCompression : std::uint8_t { zlib_deflate = 1 };

struct StoredTile {
    image::TileCoordinate coordinate{};
    image::TileExtent extent{};
    TileCompression compression{TileCompression::zlib_deflate};
    std::vector<std::byte> pixels;
    friend bool operator==(const StoredTile&, const StoredTile&) = default;
};

struct StoredTiledImage {
    std::string resource_id;
    std::uint32_t width{};
    std::uint32_t height{};
    std::uint32_t tile_size{};
    image::PixelFormat format{};
    std::vector<std::byte> clear_pixel;
    std::vector<StoredTile> occupied_tiles;
    friend bool operator==(const StoredTiledImage&, const StoredTiledImage&) = default;
};

struct OpaqueContainerSection {
    std::uint32_t kind{};
    std::uint32_t version{};
    std::vector<std::byte> payload;
    friend bool operator==(const OpaqueContainerSection&, const OpaqueContainerSection&) = default;
};

struct ProjectResource {
    std::string identifier;
    std::string kind;
    std::string relative_path;
    std::optional<std::vector<std::byte>> packed_bytes;
    friend bool operator==(const ProjectResource&, const ProjectResource&) = default;
};

struct ProjectContainer {
    ContainerSchemaVersion schema_version{current_container_schema};
    std::vector<StoredTiledImage> tiled_images;
    std::vector<ProjectResource> resources;
    std::vector<OpaqueContainerSection> opaque_sections;
};

enum class ProjectResourceStatus : std::uint8_t { packed, referenced, missing };

struct ResolvedProjectResource {
    std::string identifier;
    std::string kind;
    std::string relative_path;
    ProjectResourceStatus status{};
    std::vector<std::byte> bytes;
};

struct ProjectResourceResolution {
    std::vector<ResolvedProjectResource> resources;
    std::vector<std::string> missing_identifiers;
    [[nodiscard]] bool complete() const noexcept { return missing_identifiers.empty(); }
};

struct UnknownContainerPart {
    std::uint32_t section_kind{};
    std::uint32_t section_version{};
    std::size_t payload_bytes{};
    std::string message;
    friend bool operator==(const UnknownContainerPart&, const UnknownContainerPart&) = default;
};

struct ContainerReadReport {
    ContainerSchemaVersion source_schema{};
    bool newer_schema{};
    std::vector<UnknownContainerPart> unknown_parts;
};

struct ProjectContainerReadResult {
    ProjectContainer container;
    ContainerReadReport report;
};

enum class ProjectContainerErrorCode : std::uint8_t {
    malformed_header,
    malformed_section,
    invalid_tile,
    invalid_resource,
    compression_failed,
    over_limit,
    filesystem_failure,
};

class ProjectContainerError : public std::runtime_error {
public:
    ProjectContainerError(ProjectContainerErrorCode code, std::string message);
    [[nodiscard]] ProjectContainerErrorCode code() const noexcept { return code_; }

private:
    ProjectContainerErrorCode code_;
};

struct ProjectContainerReadLimits {
    std::size_t maximum_sections{1'000'000};
    std::size_t maximum_images{1'000'000};
    std::size_t maximum_tiles{16'000'000};
    std::size_t maximum_resources{1'000'000};
    std::size_t maximum_string_bytes{1ULL << 20};
    std::size_t maximum_decoded_tile_bytes{1ULL << 30};
    std::size_t maximum_packed_resource_bytes{1ULL << 30};
};

[[nodiscard]] ContainerSchemaVersion probe_project_container_version(
    std::span<const std::byte> bytes);
[[nodiscard]] std::vector<std::byte> write_project_container(const ProjectContainer& container);
void save_project_container_atomic(const std::filesystem::path& path,
                                   const ProjectContainer& container);
[[nodiscard]] ProjectContainerReadResult read_project_container(
    std::span<const std::byte> bytes, ProjectContainerReadLimits limits = {});

[[nodiscard]] StoredTiledImage snapshot_tiled_image(std::string resource_id,
                                                    const image::TiledImage& image);
[[nodiscard]] image::TiledImage restore_tiled_image(const StoredTiledImage& stored);
[[nodiscard]] ProjectResourceResolution resolve_project_resources(
    const ProjectContainer& container, const std::filesystem::path& project_directory);

}  // namespace ctex::io

#endif
