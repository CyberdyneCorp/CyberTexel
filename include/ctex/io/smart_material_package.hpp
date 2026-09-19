#ifndef CTEX_IO_SMART_MATERIAL_PACKAGE_HPP
#define CTEX_IO_SMART_MATERIAL_PACKAGE_HPP

#include <ctex/doc/smart_material.hpp>
#include <ctex/io/standalone_asset.hpp>

namespace ctex::io {

struct SmartMaterialImageInputResolution {
    doc::SmartMaterialImageResourceUse input;
    ProjectResourceStatus status{ProjectResourceStatus::missing};
    friend bool operator==(const SmartMaterialImageInputResolution&,
                           const SmartMaterialImageInputResolution&) = default;
};

struct SmartMaterialImportResult {
    doc::SmartMaterialPreset material;
    StandaloneAssetImportResult asset;
    std::vector<SmartMaterialImageInputResolution> image_inputs;

    [[nodiscard]] bool resources_complete() const noexcept { return asset.resources.complete(); }
};

[[nodiscard]] ProjectContainer package_smart_material(
    const doc::SmartMaterialPreset& material, std::span<const ProjectResource> resources,
    const StandaloneAssetExportOptions& options = {});

[[nodiscard]] SmartMaterialImportResult import_smart_material(
    std::span<const std::byte> bytes, std::span<const std::filesystem::path> resource_search_paths,
    ProjectContainerReadLimits limits = {});

}  // namespace ctex::io

#endif
