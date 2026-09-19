#include <algorithm>
#include <ctex/io/smart_material_package.hpp>
#include <iterator>
#include <map>
#include <utility>

namespace ctex::io {
namespace {

std::vector<std::byte> payload_bytes(std::string_view value) {
    std::vector<std::byte> result;
    result.reserve(value.size());
    for (const unsigned char byte : value) {
        result.push_back(static_cast<std::byte>(byte));
    }
    return result;
}

std::string payload_text(std::span<const std::byte> value) {
    std::string result;
    result.reserve(value.size());
    for (const std::byte byte : value) {
        result.push_back(static_cast<char>(std::to_integer<unsigned char>(byte)));
    }
    return result;
}

const ProjectResource& find_resource(std::span<const ProjectResource> resources,
                                     std::string_view identifier) {
    const auto found = std::find_if(
        resources.begin(), resources.end(),
        [&](const ProjectResource& resource) { return resource.identifier == identifier; });
    if (found == resources.end() ||
        std::find_if(std::next(found), resources.end(), [&](const ProjectResource& resource) {
            return resource.identifier == identifier;
        }) != resources.end()) {
        throw ProjectContainerError(
            ProjectContainerErrorCode::invalid_resource,
            "smart material resource is absent or ambiguous: " + std::string(identifier));
    }
    return *found;
}

void validate_package_manifest(const doc::SmartMaterialPreset& material,
                               const ProjectContainer& package) {
    const StandaloneAsset& asset = package.assets.front();
    if (asset.kind != asset_kind::smart_material || asset.identifier != material.identifier ||
        asset.format_version != material.schema_version ||
        asset.resource_dependencies.size() != material.resource_references.size() ||
        package.resources.size() != material.resource_references.size() ||
        !asset.tiled_image_dependencies.empty() || !package.tiled_images.empty()) {
        throw ProjectContainerError(ProjectContainerErrorCode::invalid_asset,
                                    "smart material package manifest does not match its payload");
    }
    for (std::size_t index = 0; index < material.resource_references.size(); ++index) {
        const doc::SmartMaterialResourceReference& expected = material.resource_references[index];
        if (asset.resource_dependencies[index] != expected.identifier ||
            find_resource(package.resources, expected.identifier).kind != expected.kind) {
            throw ProjectContainerError(
                ProjectContainerErrorCode::invalid_asset,
                "smart material package resource does not match its payload");
        }
    }
}

std::vector<SmartMaterialImageInputResolution> resolve_inputs(
    const doc::SmartMaterialPreset& material, const ProjectResourceResolution& resources) {
    std::map<std::string_view, ProjectResourceStatus, std::less<>> statuses;
    for (const ResolvedProjectResource& resource : resources.resources) {
        statuses.emplace(resource.identifier, resource.status);
    }
    std::vector<SmartMaterialImageInputResolution> result;
    for (doc::SmartMaterialImageResourceUse use :
         doc::list_smart_material_image_resource_uses(material)) {
        const ProjectResourceStatus status = statuses.at(use.resource_identifier);
        result.push_back({.input = std::move(use), .status = status});
    }
    return result;
}

}  // namespace

ProjectContainer package_smart_material(const doc::SmartMaterialPreset& material,
                                        std::span<const ProjectResource> resources,
                                        const StandaloneAssetExportOptions& options) {
    doc::validate_smart_material(material);
    ProjectContainer source;
    StandaloneAsset asset{.identifier = material.identifier,
                          .kind = std::string(asset_kind::smart_material),
                          .format_version = material.schema_version,
                          .resource_dependencies = {},
                          .tiled_image_dependencies = {},
                          .payload = payload_bytes(doc::serialize_smart_material(material))};
    asset.resource_dependencies.reserve(material.resource_references.size());
    source.resources.reserve(material.resource_references.size());
    for (const doc::SmartMaterialResourceReference& reference : material.resource_references) {
        const ProjectResource& resource = find_resource(resources, reference.identifier);
        if (resource.kind != reference.kind) {
            throw ProjectContainerError(
                ProjectContainerErrorCode::invalid_resource,
                "smart material resource kind does not match declaration: " + reference.identifier);
        }
        asset.resource_dependencies.push_back(reference.identifier);
        source.resources.push_back(resource);
    }
    source.assets.push_back(std::move(asset));
    return package_standalone_asset(source, material.identifier, options);
}

SmartMaterialImportResult import_smart_material(
    std::span<const std::byte> bytes, std::span<const std::filesystem::path> resource_search_paths,
    ProjectContainerReadLimits limits) {
    StandaloneAssetImportResult imported =
        import_standalone_asset(bytes, resource_search_paths, limits);
    const StandaloneAsset& asset = imported.package.assets.front();
    if (asset.kind != asset_kind::smart_material) {
        throw ProjectContainerError(ProjectContainerErrorCode::invalid_asset,
                                    "standalone asset is not a smart material");
    }
    doc::SmartMaterialPreset material =
        doc::deserialize_smart_material(payload_text(asset.payload));
    validate_package_manifest(material, imported.package);
    std::vector<SmartMaterialImageInputResolution> image_inputs =
        resolve_inputs(material, imported.resources);
    return {.material = std::move(material),
            .asset = std::move(imported),
            .image_inputs = std::move(image_inputs)};
}

}  // namespace ctex::io
