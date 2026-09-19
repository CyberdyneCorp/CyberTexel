#include <algorithm>
#include <ctex/doc/smart_material.hpp>
#include <map>
#include <set>
#include <utility>

namespace ctex::doc {
namespace {

[[noreturn]] void resource_error(std::string message) {
    throw SmartMaterialError(SmartMaterialErrorCode::invalid_resource_reference,
                             std::move(message));
}

bool portable_identifier(std::string_view identifier) {
    if (identifier.empty() || identifier.front() == '/' || identifier.front() == '\\' ||
        identifier.find(':') != std::string_view::npos) {
        return false;
    }
    std::size_t begin = 0;
    while (begin <= identifier.size()) {
        const std::size_t end = identifier.find_first_of("/\\", begin);
        const std::string_view component = identifier.substr(begin, end - begin);
        if (component.empty() || component == "." || component == "..") {
            return false;
        }
        if (end == std::string_view::npos) {
            return true;
        }
        begin = end + 1;
    }
    return true;
}

void append_image_use(std::vector<SmartMaterialImageResourceUse>& uses,
                      const SmartMaterialEntry& entry, graph::NodeId node_id,
                      SmartMaterialResourceLocation location, std::string_view target,
                      const graph::SocketValue& value) {
    const auto* image = std::get_if<graph::ImageValue>(&value);
    if (image == nullptr || image->resource_id.empty()) {
        return;
    }
    uses.push_back({.entry_identifier = entry.identifier,
                    .node_id = node_id,
                    .location = location,
                    .target_identifier = std::string(target),
                    .resource_identifier = image->resource_id});
}

std::vector<SmartMaterialImageResourceUse> collect_image_uses(const SmartMaterialPreset& preset) {
    std::vector<SmartMaterialImageResourceUse> result;
    for (const SmartMaterialEntry& entry : preset.stack) {
        if (!entry.graph.has_value()) {
            continue;
        }
        for (const graph::GraphNode& node : entry.graph->nodes()) {
            for (const graph::NodeSocket& input : node.inputs) {
                append_image_use(result, entry, node.id, SmartMaterialResourceLocation::input,
                                 input.identifier, input.value);
            }
            for (const graph::NodeSocket& output : node.outputs) {
                append_image_use(result, entry, node.id, SmartMaterialResourceLocation::output,
                                 output.identifier, output.value);
            }
            for (const graph::NodeProperty& property : node.properties) {
                append_image_use(result, entry, node.id, SmartMaterialResourceLocation::property,
                                 property.key, property.value);
            }
        }
    }
    return result;
}

void require_image_resource(
    const std::map<std::string_view, std::string_view, std::less<>>& resources,
    std::string_view identifier, std::string subject) {
    if (identifier.empty()) {
        return;
    }
    const auto resource = resources.find(identifier);
    if (resource == resources.end() || resource->second != "image") {
        resource_error(std::move(subject) + " references undeclared image resource '" +
                       std::string(identifier) + "'");
    }
}

}  // namespace

void validate_smart_material_resources(const SmartMaterialPreset& preset) {
    std::map<std::string_view, std::string_view, std::less<>> resources;
    for (const SmartMaterialResourceReference& resource : preset.resource_references) {
        if (!portable_identifier(resource.identifier) || resource.kind.empty() ||
            !resources.emplace(resource.identifier, resource.kind).second) {
            resource_error("smart material resources require unique portable identities and kinds");
        }
    }
    for (const SmartMaterialImageResourceUse& use : collect_image_uses(preset)) {
        require_image_resource(resources, use.resource_identifier, "graph image");
    }
    for (const ExposedSmartMaterialParameter& parameter : preset.exposed_parameters) {
        if (const auto* image = std::get_if<graph::ImageValue>(&parameter.default_value)) {
            require_image_resource(resources, image->resource_id,
                                   "image parameter '" + parameter.identifier + "'");
        }
    }
}

std::vector<SmartMaterialImageResourceUse> list_smart_material_image_resource_uses(
    const SmartMaterialPreset& preset) {
    validate_smart_material(preset);
    return collect_image_uses(preset);
}

}  // namespace ctex::doc
