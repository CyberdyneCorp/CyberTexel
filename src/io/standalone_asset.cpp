#include <algorithm>
#include <ctex/io/standalone_asset.hpp>
#include <iterator>
#include <string>

namespace ctex::io {
namespace {

const StandaloneAsset& find_asset(const ProjectContainer& source, std::string_view identifier) {
    const auto found =
        std::find_if(source.assets.begin(), source.assets.end(),
                     [&](const StandaloneAsset& asset) { return asset.identifier == identifier; });
    if (found == source.assets.end() ||
        std::find_if(std::next(found), source.assets.end(), [&](const StandaloneAsset& asset) {
            return asset.identifier == identifier;
        }) != source.assets.end()) {
        throw ProjectContainerError(ProjectContainerErrorCode::invalid_asset,
                                    "standalone asset identity is absent or ambiguous");
    }
    return *found;
}

template <typename Entry, typename Identity>
const Entry& find_dependency(const std::vector<Entry>& entries, std::string_view identifier,
                             Identity identity, std::string_view kind) {
    const auto found = std::find_if(entries.begin(), entries.end(), [&](const Entry& entry) {
        return identity(entry) == identifier;
    });
    if (found == entries.end() ||
        std::find_if(std::next(found), entries.end(), [&](const Entry& entry) {
            return identity(entry) == identifier;
        }) != entries.end()) {
        throw ProjectContainerError(
            ProjectContainerErrorCode::invalid_asset,
            "standalone asset " + std::string(kind) +
                " dependency is absent or ambiguous: " + std::string(identifier));
    }
    return *found;
}

ProjectResource pack_resource(ProjectResource resource,
                              const std::filesystem::path& source_directory) {
    if (resource.packed_bytes.has_value()) {
        return resource;
    }
    ProjectContainer lookup;
    lookup.resources.push_back(resource);
    ProjectResourceResolution resolution = resolve_project_resources(lookup, source_directory);
    if (!resolution.complete()) {
        throw ProjectContainerError(
            ProjectContainerErrorCode::invalid_resource,
            "self-contained asset resource is missing: " + resource.identifier);
    }
    resource.packed_bytes = std::move(resolution.resources.front().bytes);
    return resource;
}

void validate_import_dependencies(const ProjectContainer& package) {
    if (package.assets.size() != 1) {
        throw ProjectContainerError(ProjectContainerErrorCode::invalid_asset,
                                    "standalone asset package must contain exactly one asset");
    }
    const StandaloneAsset& asset = package.assets.front();
    for (const std::string& identifier : asset.resource_dependencies) {
        static_cast<void>(find_dependency(
            package.resources, identifier,
            [](const ProjectResource& resource) -> std::string_view { return resource.identifier; },
            "resource"));
    }
    for (const std::string& identifier : asset.tiled_image_dependencies) {
        static_cast<void>(find_dependency(
            package.tiled_images, identifier,
            [](const StoredTiledImage& image) -> std::string_view { return image.resource_id; },
            "tiled image"));
    }
}

bool is_self_contained(const ProjectContainer& package) {
    const StandaloneAsset& asset = package.assets.front();
    return std::all_of(asset.resource_dependencies.begin(), asset.resource_dependencies.end(),
                       [&](const std::string& identifier) {
                           return find_dependency(
                                      package.resources, identifier,
                                      [](const ProjectResource& resource) -> std::string_view {
                                          return resource.identifier;
                                      },
                                      "resource")
                               .packed_bytes.has_value();
                       });
}

const ResolvedProjectResource& find_resolved_resource(const ProjectResourceResolution& resolution,
                                                      std::string_view identifier) {
    const auto found = std::find_if(
        resolution.resources.begin(), resolution.resources.end(),
        [&](const ResolvedProjectResource& resource) { return resource.identifier == identifier; });
    if (found == resolution.resources.end()) {
        throw ProjectContainerError(
            ProjectContainerErrorCode::invalid_resource,
            "imported asset resource was not resolved: " + std::string(identifier));
    }
    return *found;
}

template <typename Entry, typename Identity>
bool append_or_reuse(std::vector<Entry>& destination, Entry entry, Identity identity) {
    const auto existing = std::find_if(
        destination.begin(), destination.end(),
        [&](const Entry& candidate) { return identity(candidate) == identity(entry); });
    if (existing == destination.end()) {
        destination.push_back(std::move(entry));
        return true;
    }
    if (*existing != entry) {
        throw ProjectContainerError(ProjectContainerErrorCode::invalid_asset,
                                    "asset dependency conflicts with the library identity");
    }
    return false;
}

}  // namespace

ProjectContainer package_standalone_asset(const ProjectContainer& source,
                                          std::string_view asset_identifier,
                                          const StandaloneAssetExportOptions& options) {
    const StandaloneAsset& asset = find_asset(source, asset_identifier);
    ProjectContainer package{.schema_version = source.schema_version,
                             .tiled_images = {},
                             .resources = {},
                             .assets = {asset},
                             .opaque_sections = source.opaque_sections};
    package.resources.reserve(asset.resource_dependencies.size());
    for (const std::string& identifier : asset.resource_dependencies) {
        ProjectResource resource = find_dependency(
            source.resources, identifier,
            [](const ProjectResource& candidate) -> std::string_view {
                return candidate.identifier;
            },
            "resource");
        if (options.self_contained) {
            resource = pack_resource(std::move(resource), options.source_directory);
        }
        package.resources.push_back(std::move(resource));
    }
    package.tiled_images.reserve(asset.tiled_image_dependencies.size());
    for (const std::string& identifier : asset.tiled_image_dependencies) {
        package.tiled_images.push_back(find_dependency(
            source.tiled_images, identifier,
            [](const StoredTiledImage& candidate) -> std::string_view {
                return candidate.resource_id;
            },
            "tiled image"));
    }
    static_cast<void>(write_project_container(package));
    return package;
}

void save_standalone_asset_atomic(const std::filesystem::path& path, const ProjectContainer& source,
                                  std::string_view asset_identifier,
                                  const StandaloneAssetExportOptions& options) {
    save_project_container_atomic(path,
                                  package_standalone_asset(source, asset_identifier, options));
}

StandaloneAssetImportResult import_standalone_asset(std::span<const std::byte> bytes,
                                                    const std::filesystem::path& package_directory,
                                                    ProjectContainerReadLimits limits) {
    ProjectContainerReadResult opened = read_project_container(bytes, limits);
    validate_import_dependencies(opened.container);
    ProjectResourceResolution resources =
        resolve_project_resources(opened.container, package_directory);
    const bool self_contained = is_self_contained(opened.container);
    return {.package = std::move(opened.container),
            .read_report = std::move(opened.report),
            .resources = std::move(resources),
            .self_contained = self_contained};
}

StandaloneAssetInstallReport install_standalone_asset(ProjectContainer& library,
                                                      StandaloneAssetImportResult imported) {
    validate_import_dependencies(imported.package);
    StandaloneAsset asset = imported.package.assets.front();
    if (std::any_of(library.assets.begin(), library.assets.end(), [&](const StandaloneAsset& item) {
            return item.identifier == asset.identifier;
        })) {
        throw ProjectContainerError(ProjectContainerErrorCode::invalid_asset,
                                    "asset identity already exists in the library");
    }

    ProjectContainer candidate = library;
    StandaloneAssetInstallReport report{.asset_identifier = asset.identifier,
                                        .added_resources = {},
                                        .reused_resources = {},
                                        .added_tiled_images = {},
                                        .reused_tiled_images = {}};
    for (const std::string& identifier : asset.resource_dependencies) {
        ProjectResource resource = find_dependency(
            imported.package.resources, identifier,
            [](const ProjectResource& item) -> std::string_view { return item.identifier; },
            "resource");
        if (!resource.packed_bytes.has_value()) {
            const ResolvedProjectResource& resolved =
                find_resolved_resource(imported.resources, identifier);
            if (resolved.status == ProjectResourceStatus::missing) {
                throw ProjectContainerError(
                    ProjectContainerErrorCode::invalid_resource,
                    "asset cannot be installed with a missing resource: " + identifier);
            }
            resource.packed_bytes = resolved.bytes;
        }
        if (append_or_reuse(
                candidate.resources, std::move(resource),
                [](const ProjectResource& item) -> std::string_view { return item.identifier; })) {
            report.added_resources.push_back(identifier);
        } else {
            report.reused_resources.push_back(identifier);
        }
    }
    for (const std::string& identifier : asset.tiled_image_dependencies) {
        StoredTiledImage image = find_dependency(
            imported.package.tiled_images, identifier,
            [](const StoredTiledImage& item) -> std::string_view { return item.resource_id; },
            "tiled image");
        if (append_or_reuse(candidate.tiled_images, std::move(image),
                            [](const StoredTiledImage& item) -> std::string_view {
                                return item.resource_id;
                            })) {
            report.added_tiled_images.push_back(identifier);
        } else {
            report.reused_tiled_images.push_back(identifier);
        }
    }
    candidate.assets.push_back(std::move(asset));
    candidate.opaque_sections.insert(candidate.opaque_sections.end(),
                                     imported.package.opaque_sections.begin(),
                                     imported.package.opaque_sections.end());
    static_cast<void>(write_project_container(candidate));
    library = std::move(candidate);
    return report;
}

}  // namespace ctex::io
