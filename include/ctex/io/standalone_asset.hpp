#ifndef CTEX_IO_STANDALONE_ASSET_HPP
#define CTEX_IO_STANDALONE_ASSET_HPP

#include <ctex/io/project_container.hpp>
#include <filesystem>
#include <span>
#include <string_view>

namespace ctex::io {

namespace asset_kind {
inline constexpr std::string_view material = "material";
inline constexpr std::string_view smart_material = "smart-material";
inline constexpr std::string_view smart_mask = "smart-mask";
inline constexpr std::string_view brush = "brush";
inline constexpr std::string_view stroke_preset = "stroke-preset";
inline constexpr std::string_view export_preset = "export-preset";
inline constexpr std::string_view generator = "generator";
inline constexpr std::string_view node_group = "node-group";
}  // namespace asset_kind

struct StandaloneAssetExportOptions {
    bool self_contained{};
    std::filesystem::path source_directory;
};

[[nodiscard]] ProjectContainer package_standalone_asset(
    const ProjectContainer& source, std::string_view asset_identifier,
    const StandaloneAssetExportOptions& options = {});
void save_standalone_asset_atomic(const std::filesystem::path& path, const ProjectContainer& source,
                                  std::string_view asset_identifier,
                                  const StandaloneAssetExportOptions& options = {});

struct StandaloneAssetImportResult {
    ProjectContainer package;
    ContainerReadReport read_report;
    ProjectResourceResolution resources;
    bool self_contained{};
};

[[nodiscard]] StandaloneAssetImportResult import_standalone_asset(
    std::span<const std::byte> bytes, const std::filesystem::path& package_directory,
    ProjectContainerReadLimits limits = {});
[[nodiscard]] StandaloneAssetImportResult import_standalone_asset(
    std::span<const std::byte> bytes, std::span<const std::filesystem::path> resource_search_paths,
    ProjectContainerReadLimits limits = {});

struct StandaloneAssetInstallReport {
    std::string asset_identifier;
    std::vector<std::string> added_resources;
    std::vector<std::string> reused_resources;
    std::vector<std::string> added_tiled_images;
    std::vector<std::string> reused_tiled_images;
};

[[nodiscard]] StandaloneAssetInstallReport install_standalone_asset(
    ProjectContainer& library, StandaloneAssetImportResult imported);

}  // namespace ctex::io

#endif
