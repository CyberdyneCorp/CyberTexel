#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <ctex/io/editable_authoring.hpp>
#include <ctex/io/texture_document.hpp>
#include <limits>
#include <map>
#include <set>
#include <span>
#include <utility>

namespace ctex::io {

struct TextureDocumentArchiveAccess {
    static void restore_applications(doc::TextureSet& texture_set,
                                     std::vector<doc::AppliedPresetApplication> applications) {
        texture_set.preset_applications_.assign(std::make_move_iterator(applications.begin()),
                                                std::make_move_iterator(applications.end()));
    }
};

namespace {

constexpr std::array<std::byte, 8> document_magic{std::byte{'C'}, std::byte{'T'}, std::byte{'E'},
                                                  std::byte{'X'}, std::byte{'D'}, std::byte{'O'},
                                                  std::byte{'C'}, std::byte{0}};

class Writer {
public:
    void u8(std::uint8_t value) { bytes_.push_back(static_cast<std::byte>(value)); }
    void u32(std::uint32_t value) {
        for (unsigned shift = 0; shift < 32; shift += 8) u8(value >> shift);
    }
    void u64(std::uint64_t value) {
        for (unsigned shift = 0; shift < 64; shift += 8) u8(value >> shift);
    }
    void f64(double value) {
        if (!std::isfinite(value)) {
            throw TextureDocumentIoError("texture document contains a non-finite number");
        }
        u64(std::bit_cast<std::uint64_t>(value));
    }
    void bytes(std::span<const std::byte> value) {
        bytes_.insert(bytes_.end(), value.begin(), value.end());
    }
    void string(std::string_view value) {
        if (value.size() > std::numeric_limits<std::uint32_t>::max()) {
            throw TextureDocumentIoError("texture-document string exceeds the format limit");
        }
        u32(static_cast<std::uint32_t>(value.size()));
        bytes(std::as_bytes(std::span(value)));
    }
    void blob(std::span<const std::byte> value) {
        u64(value.size());
        bytes(value);
    }
    [[nodiscard]] std::vector<std::byte> finish() && { return std::move(bytes_); }

private:
    std::vector<std::byte> bytes_;
};

class Reader {
public:
    Reader(std::span<const std::byte> bytes, const TextureDocumentReadLimits& limits)
        : bytes_(bytes), limits_(limits) {
        if (bytes.size() > limits.maximum_payload_bytes) {
            throw TextureDocumentIoError(
                "texture-document payload exceeds the configured byte limit");
        }
    }

    [[nodiscard]] std::span<const std::byte> take(std::size_t size, std::string_view field) {
        if (size > bytes_.size() - offset_) {
            throw TextureDocumentIoError("texture document ends inside " + std::string(field));
        }
        const auto result = bytes_.subspan(offset_, size);
        offset_ += size;
        return result;
    }
    [[nodiscard]] std::uint8_t u8(std::string_view field) {
        return std::to_integer<std::uint8_t>(take(1, field).front());
    }
    [[nodiscard]] std::uint32_t u32(std::string_view field) {
        const auto value = take(4, field);
        std::uint32_t result{};
        for (unsigned index = 0; index < 4; ++index) {
            result |= static_cast<std::uint32_t>(std::to_integer<std::uint8_t>(value[index]))
                      << (index * 8U);
        }
        return result;
    }
    [[nodiscard]] std::uint64_t u64(std::string_view field) {
        const auto value = take(8, field);
        std::uint64_t result{};
        for (unsigned index = 0; index < 8; ++index) {
            result |= static_cast<std::uint64_t>(std::to_integer<std::uint8_t>(value[index]))
                      << (index * 8U);
        }
        return result;
    }
    [[nodiscard]] double f64(std::string_view field) {
        const double value = std::bit_cast<double>(u64(field));
        if (!std::isfinite(value)) {
            throw TextureDocumentIoError("texture document contains non-finite " +
                                         std::string(field));
        }
        return value;
    }
    [[nodiscard]] std::string string(std::string_view field) {
        const std::size_t size = u32(field);
        if (size > limits_.maximum_string_bytes) {
            throw TextureDocumentIoError("texture-document string exceeds configured limit");
        }
        const auto value = take(size, field);
        return {reinterpret_cast<const char*>(value.data()), value.size()};
    }
    [[nodiscard]] std::span<const std::byte> blob(std::string_view field) {
        const std::uint64_t declared = u64(field);
        if (declared > limits_.maximum_embedded_blob_bytes ||
            declared > std::numeric_limits<std::size_t>::max()) {
            throw TextureDocumentIoError(std::string(field) + " exceeds configured limit");
        }
        return take(static_cast<std::size_t>(declared), field);
    }
    [[nodiscard]] bool empty() const noexcept { return offset_ == bytes_.size(); }
    [[nodiscard]] std::size_t remaining() const noexcept { return bytes_.size() - offset_; }
    void require_records(std::size_t count, std::size_t minimum_bytes,
                         std::string_view field) const {
        if (minimum_bytes != 0 && count > remaining() / minimum_bytes) {
            throw TextureDocumentIoError(std::string(field) +
                                         " cannot fit in the remaining payload");
        }
    }

private:
    std::span<const std::byte> bytes_;
    const TextureDocumentReadLimits& limits_;
    std::size_t offset_{};
};

void require_count(std::size_t count, std::size_t limit, std::size_t& total,
                   std::string_view field) {
    if (count > limit - std::min(limit, total)) {
        throw TextureDocumentIoError(std::string(field) + " exceeds configured limit");
    }
    total += count;
}

std::uint32_t format_count(std::size_t count, std::string_view field) {
    if (count > std::numeric_limits<std::uint32_t>::max()) {
        throw TextureDocumentIoError(std::string(field) + " exceeds the format limit");
    }
    return static_cast<std::uint32_t>(count);
}

bool read_bool(Reader& reader, std::string_view field) {
    const std::uint8_t value = reader.u8(field);
    if (value > 1) {
        throw TextureDocumentIoError(std::string(field) + " is not a boolean");
    }
    return value != 0;
}

template <typename Enum>
Enum read_enum(Reader& reader, std::uint8_t maximum, std::string_view field) {
    const std::uint8_t value = reader.u8(field);
    if (value > maximum) {
        throw TextureDocumentIoError(std::string(field) + " is invalid");
    }
    return static_cast<Enum>(value);
}

std::string encoded_segment(std::string_view value) {
    constexpr char hexadecimal[] = "0123456789abcdef";
    std::string result;
    result.reserve(value.size() * 2);
    for (const unsigned char byte : value) {
        result.push_back(hexadecimal[byte >> 4U]);
        result.push_back(hexadecimal[byte & 0x0fU]);
    }
    return result;
}

std::string image_identity(std::string_view asset_identifier, std::string_view texture_set,
                           std::string_view channel, std::optional<std::uint32_t> udim) {
    std::string result =
        std::string(asset_identifier) + "/pixels/" + encoded_segment(texture_set) + '/';
    if (udim) {
        result += "udim/" + std::to_string(*udim) + '/';
    } else {
        result += "base/";
    }
    return result + encoded_segment(channel);
}

struct PackageContext {
    std::string_view asset_identifier;
    std::vector<StoredTiledImage> images;
    std::set<std::string, std::less<>> resource_dependencies;
};

void write_channel_descriptor(Writer& writer, const doc::ChannelDescriptor& descriptor) {
    writer.string(descriptor.semantic_id);
    writer.u8(descriptor.component_count);
    writer.u8(static_cast<std::uint8_t>(descriptor.scalar_representation));
    writer.u8(descriptor.preferred_bit_depth);
    writer.u8(static_cast<std::uint8_t>(descriptor.classification));
    writer.u8(static_cast<std::uint8_t>(descriptor.blending_policy));
    writer.u8(descriptor.evaluable ? 1 : 0);
    writer.u8(0);
    writer.u8(0);
    writer.string(descriptor.export_mapping);
    writer.u32(format_count(descriptor.default_value.size(), "channel default component count"));
    for (double value : descriptor.default_value) writer.f64(value);
}

doc::ChannelDescriptor read_channel_descriptor(Reader& reader,
                                               std::pmr::memory_resource* memory_resource) {
    doc::ChannelDescriptor descriptor{
        .semantic_id = std::pmr::string(reader.string("channel identity"), memory_resource),
        .component_count = reader.u8("channel component count"),
        .scalar_representation =
            read_enum<doc::ScalarRepresentation>(reader, 1, "channel scalar representation"),
        .preferred_bit_depth = reader.u8("channel preferred bit depth"),
        .default_value = std::pmr::vector<double>(memory_resource),
        .classification =
            read_enum<doc::ChannelClassification>(reader, 1, "channel classification"),
        .blending_policy = read_enum<doc::BlendingPolicy>(reader, 3, "channel blending policy"),
        .export_mapping = {},
        .evaluable = read_bool(reader, "channel evaluable flag"),
    };
    static_cast<void>(reader.take(2, "channel reserved bytes"));
    descriptor.export_mapping =
        std::pmr::string(reader.string("channel export mapping"), memory_resource);
    const std::uint32_t default_count = reader.u32("channel default component count");
    if (default_count > 4) {
        throw TextureDocumentIoError("channel default component count is invalid");
    }
    descriptor.default_value.reserve(default_count);
    for (std::uint32_t index = 0; index < default_count; ++index) {
        descriptor.default_value.push_back(reader.f64("channel default value"));
    }
    return descriptor;
}

bool equal_descriptor(const doc::ChannelDescriptor& left, const doc::ChannelDescriptor& right) {
    return left.semantic_id == right.semantic_id && left.component_count == right.component_count &&
           left.scalar_representation == right.scalar_representation &&
           left.preferred_bit_depth == right.preferred_bit_depth &&
           std::ranges::equal(left.default_value, right.default_value) &&
           left.classification == right.classification &&
           left.blending_policy == right.blending_policy &&
           left.export_mapping == right.export_mapping && left.evaluable == right.evaluable;
}

void collect_socket_resource(const graph::SocketValue& value,
                             std::set<std::string, std::less<>>& resources) {
    if (const auto* image = std::get_if<graph::ImageValue>(&value);
        image != nullptr && !image->resource_id.empty()) {
        resources.insert(image->resource_id);
    }
}

void collect_graph_resources(const graph::GraphDocument& graph,
                             std::set<std::string, std::less<>>& resources) {
    for (const graph::GraphNode& node : graph.nodes()) {
        for (const graph::NodeSocket& socket : node.inputs) {
            collect_socket_resource(socket.value, resources);
        }
        for (const graph::NodeSocket& socket : node.outputs) {
            collect_socket_resource(socket.value, resources);
        }
        for (const graph::NodeProperty& property : node.properties) {
            collect_socket_resource(property.value, resources);
        }
    }
}

void write_channel_pixels(Writer& writer, const doc::TextureChannels& channels,
                          std::string_view texture_set_id, std::optional<std::uint32_t> udim,
                          PackageContext& context, bool write_descriptors) {
    const std::vector<std::string> semantic_ids = channels.semantic_ids();
    writer.u32(format_count(semantic_ids.size(), "channel count"));
    for (const std::string& semantic_id : semantic_ids) {
        if (write_descriptors) {
            write_channel_descriptor(writer, channels.descriptor(semantic_id));
        } else {
            writer.string(semantic_id);
        }
        const bool enabled = channels.is_enabled(semantic_id);
        writer.u8(enabled ? 1 : 0);
        if (enabled) {
            const std::string identity =
                image_identity(context.asset_identifier, texture_set_id, semantic_id, udim);
            writer.string(identity);
            context.images.push_back(snapshot_tiled_image(identity, channels.pixels(semantic_id)));
        }
    }
}

void write_layer_entry(Writer& writer, const doc::LayerEntry& entry, PackageContext& context) {
    writer.string(entry.identifier);
    writer.string(entry.display_name);
    writer.u8(static_cast<std::uint8_t>(entry.kind));
    writer.u8(entry.enabled ? 1 : 0);
    writer.u8(entry.graph.has_value() ? 1 : 0);
    writer.u8(0);
    writer.string(entry.parent_identifier);
    writer.string(entry.target_identifier);
    writer.string(entry.source_identifier);
    writer.f64(entry.opacity);
    writer.string(entry.blend_mode);
    writer.u64(entry.content_revision);
    writer.u32(format_count(entry.channels.size(), "layer channel modulation count"));
    for (const doc::LayerEntry::ChannelModulation& channel : entry.channels) {
        writer.string(channel.semantic_id);
        writer.u8(channel.enabled ? 1 : 0);
        writer.f64(channel.opacity);
    }
    if (entry.graph) {
        const std::string graph = graph::serialize_graph(*entry.graph);
        writer.blob(std::as_bytes(std::span(graph)));
        collect_graph_resources(*entry.graph, context.resource_dependencies);
    }
}

void write_socket_value(Writer& writer, const graph::SocketValue& value, PackageContext& context) {
    writer.u8(static_cast<std::uint8_t>(value.index()));
    std::visit(
        [&](const auto& current) {
            using Value = std::decay_t<decltype(current)>;
            if constexpr (std::is_same_v<Value, std::monostate>) {
                return;
            } else if constexpr (std::is_same_v<Value, bool>) {
                writer.u8(current ? 1 : 0);
            } else if constexpr (std::is_same_v<Value, double>) {
                writer.f64(current);
            } else if constexpr (std::is_same_v<Value, graph::VectorValue>) {
                writer.f64(current.x);
                writer.f64(current.y);
                writer.f64(current.z);
            } else if constexpr (std::is_same_v<Value, graph::ColourValue>) {
                writer.f64(current.r);
                writer.f64(current.g);
                writer.f64(current.b);
                writer.f64(current.a);
            } else if constexpr (std::is_same_v<Value, std::string>) {
                writer.string(current);
            } else {
                writer.string(current.resource_id);
                collect_socket_resource(value, context.resource_dependencies);
            }
        },
        value);
}

void write_application(Writer& writer, const doc::AppliedPresetApplication& application,
                       PackageContext& context) {
    writer.string(application.identifier);
    writer.u8(static_cast<std::uint8_t>(application.kind));
    writer.string(application.target_entry_identifier);
    writer.string(application.origin.preset_identifier);
    writer.u32(application.origin.schema_version);
    const std::string fragment = doc::serialize_smart_material(application.fragment);
    writer.blob(std::as_bytes(std::span(fragment)));
    for (const doc::SmartMaterialResourceReference& resource :
         application.fragment.resource_references) {
        context.resource_dependencies.insert(resource.identifier);
    }
    for (const doc::SmartMaterialEntry& entry : application.fragment.stack) {
        if (entry.graph) {
            collect_graph_resources(*entry.graph, context.resource_dependencies);
        }
    }
    writer.u32(format_count(application.parameter_values.size(), "preset parameter value count"));
    for (const doc::AppliedPresetParameterValue& parameter : application.parameter_values) {
        writer.string(parameter.parameter_identifier);
        write_socket_value(writer, parameter.value, context);
    }
}

void write_texture_set(Writer& writer, const doc::TextureSet& texture_set,
                       PackageContext& context) {
    const doc::TextureSetDescriptor descriptor = texture_set.descriptor();
    writer.string(texture_set.id());
    writer.string(descriptor.display_name);
    writer.u8(static_cast<std::uint8_t>(descriptor.partition_kind));
    writer.u8(descriptor.default_bit_depth);
    writer.u8(descriptor.udim_tiling ? 1 : 0);
    writer.u8(0);
    writer.string(descriptor.partition_key);
    writer.string(descriptor.uv_set);
    writer.u32(descriptor.width);
    writer.u32(descriptor.height);
    write_channel_pixels(writer, texture_set.channels(), texture_set.id(), std::nullopt, context,
                         true);

    const std::vector<std::uint32_t> udim_tiles = texture_set.occupied_udim_tiles();
    writer.u32(format_count(udim_tiles.size(), "UDIM tile count"));
    for (std::uint32_t tile : udim_tiles) {
        writer.u32(tile);
        write_channel_pixels(writer, texture_set.udim_channels(tile), texture_set.id(), tile,
                             context, false);
    }

    writer.u32(format_count(texture_set.layer_stack().size(), "layer entry count"));
    for (const doc::LayerEntry& entry : texture_set.layer_stack().entries()) {
        write_layer_entry(writer, entry, context);
    }

    writer.u32(format_count(texture_set.preset_applications().size(), "preset application count"));
    for (const doc::AppliedPresetApplication& application : texture_set.preset_applications()) {
        write_application(writer, application, context);
    }

    const std::vector<std::byte> editable =
        serialize_editable_authoring(texture_set.editable_authoring());
    writer.blob(editable);
}

void write_atlas(Writer& writer, const doc::AtlasDescriptor& atlas) {
    writer.string(atlas.identifier);
    writer.string(atlas.display_name);
    writer.u32(atlas.width);
    writer.u32(atlas.height);
    writer.u32(format_count(atlas.regions.size(), "atlas region count"));
    for (const doc::AtlasRegion& region : atlas.regions) {
        writer.string(region.texture_set_identifier);
        writer.u32(region.x);
        writer.u32(region.y);
        writer.u32(region.width);
        writer.u32(region.height);
    }
}

struct PackagedDocument {
    StandaloneAsset asset;
    std::vector<StoredTiledImage> images;
};

PackagedDocument package_document(std::string identifier, const doc::TextureDocument& document) {
    if (identifier.empty()) {
        throw TextureDocumentIoError("texture document requires a non-empty asset identity");
    }
    Writer writer;
    writer.bytes(document_magic);
    writer.u32(current_texture_document_schema);
    const std::vector<std::string> texture_sets = document.texture_set_ids();
    const std::vector<std::string> atlases = document.atlas_ids();
    writer.u32(format_count(texture_sets.size(), "texture-set count"));
    writer.u32(format_count(atlases.size(), "atlas count"));
    PackageContext context{.asset_identifier = identifier};
    for (const std::string& texture_set : texture_sets) {
        write_texture_set(writer, document.texture_set(texture_set), context);
    }
    for (const std::string& atlas : atlases) write_atlas(writer, document.atlas(atlas));
    std::ranges::sort(context.images, [](const auto& left, const auto& right) {
        return left.resource_id < right.resource_id;
    });
    std::vector<std::string> image_dependencies;
    image_dependencies.reserve(context.images.size());
    for (const StoredTiledImage& image : context.images) {
        image_dependencies.push_back(image.resource_id);
    }
    return {.asset = {.identifier = std::move(identifier),
                      .kind = std::string(texture_document_asset_kind),
                      .format_version = current_texture_document_schema,
                      .resource_dependencies = {context.resource_dependencies.begin(),
                                                context.resource_dependencies.end()},
                      .tiled_image_dependencies = std::move(image_dependencies),
                      .payload = std::move(writer).finish()},
            .images = std::move(context.images)};
}

struct ReadTotals {
    std::size_t channels{};
    std::size_t udim_tiles{};
    std::size_t layer_entries{};
    std::size_t channel_modulations{};
    std::size_t applications{};
    std::size_t parameter_values{};
    std::size_t atlas_regions{};
};

struct DecodeContext {
    const ProjectContainer& project;
    const StandaloneAsset& asset;
    const TextureDocumentReadLimits& limits;
    std::map<std::string_view, const StoredTiledImage*, std::less<>> images;
    std::set<std::string_view, std::less<>> declared_images;
    std::set<std::string, std::less<>> used_images;
    std::set<std::string_view, std::less<>> declared_resources;
    std::set<std::string, std::less<>> used_resources;
    ReadTotals totals;
};

const StoredTiledImage& require_image(DecodeContext& context, std::string_view identity) {
    if (!context.declared_images.contains(identity)) {
        throw TextureDocumentIoError("texture document uses an undeclared tiled image: " +
                                     std::string(identity));
    }
    const auto found = context.images.find(identity);
    if (found == context.images.end()) {
        throw TextureDocumentIoError("texture document tiled image is missing: " +
                                     std::string(identity));
    }
    if (!context.used_images.insert(std::string(identity)).second) {
        throw TextureDocumentIoError("texture document repeats a tiled image reference: " +
                                     std::string(identity));
    }
    return *found->second;
}

void install_descriptor(doc::TextureChannels& channels, doc::ChannelDescriptor descriptor) {
    const std::string identity(descriptor.semantic_id);
    if (channels.contains_descriptor(identity)) {
        if (!equal_descriptor(channels.descriptor(identity), descriptor)) {
            throw TextureDocumentIoError("built-in channel descriptor changed in archive: " +
                                         identity);
        }
        return;
    }
    channels.register_descriptor(std::move(descriptor));
}

void restore_pixels(Reader& reader, doc::TextureChannels& channels, DecodeContext& context,
                    bool descriptors, std::pmr::memory_resource* memory_resource) {
    const std::uint32_t channel_count = reader.u32("channel count");
    require_count(channel_count, context.limits.maximum_channels, context.totals.channels,
                  "channel count");
    reader.require_records(channel_count, descriptors ? 20 : 5, "channel count");
    std::set<std::string> identities;
    for (std::uint32_t index = 0; index < channel_count; ++index) {
        std::string identity;
        if (descriptors) {
            doc::ChannelDescriptor descriptor = read_channel_descriptor(reader, memory_resource);
            identity = std::string(descriptor.semantic_id);
            install_descriptor(channels, std::move(descriptor));
        } else {
            identity = reader.string("channel identity");
            if (!channels.contains_descriptor(identity)) {
                throw TextureDocumentIoError("UDIM tile names an unknown channel: " + identity);
            }
        }
        if (!identities.insert(identity).second) {
            throw TextureDocumentIoError("texture document repeats channel identity: " + identity);
        }
        const bool enabled = read_bool(reader, "channel enabled flag");
        if (!enabled) {
            channels.disable(identity);
            continue;
        }
        const std::string image_identity = reader.string("channel image identity");
        const image::TiledImage restored =
            restore_tiled_image(require_image(context, image_identity));
        const auto channel_type = restored.format().channel_type;
        const std::uint8_t bit_depth = channel_type == image::ChannelType::uint8_unorm    ? 8
                                       : channel_type == image::ChannelType::uint16_unorm ? 16
                                                                                          : 32;
        channels.disable(identity);
        channels.enable(identity, bit_depth);
        channels.replace_pixels(identity, restored);
    }
}

doc::LayerEntry read_layer_entry(Reader& reader, DecodeContext& context) {
    doc::LayerEntry entry;
    entry.identifier = reader.string("layer identity");
    entry.display_name = reader.string("layer display name");
    entry.kind = read_enum<doc::LayerEntryKind>(reader, 8, "layer kind");
    entry.enabled = read_bool(reader, "layer enabled flag");
    const bool has_graph = read_bool(reader, "layer graph flag");
    static_cast<void>(reader.u8("layer reserved byte"));
    entry.parent_identifier = reader.string("layer parent identity");
    entry.target_identifier = reader.string("layer target identity");
    entry.source_identifier = reader.string("layer source identity");
    entry.opacity = reader.f64("layer opacity");
    entry.blend_mode = reader.string("layer blend mode");
    entry.content_revision = reader.u64("layer content revision");
    const std::uint32_t channel_count = reader.u32("layer channel modulation count");
    require_count(channel_count, context.limits.maximum_channel_modulations,
                  context.totals.channel_modulations, "layer channel modulation count");
    entry.channels.reserve(channel_count);
    for (std::uint32_t index = 0; index < channel_count; ++index) {
        entry.channels.push_back({.semantic_id = reader.string("layer channel identity"),
                                  .enabled = read_bool(reader, "layer channel enabled flag"),
                                  .opacity = reader.f64("layer channel opacity")});
    }
    if (has_graph) {
        const auto graph_bytes = reader.blob("layer graph");
        entry.graph = graph::deserialize_graph(
            {reinterpret_cast<const char*>(graph_bytes.data()), graph_bytes.size()});
        collect_graph_resources(*entry.graph, context.used_resources);
    }
    return entry;
}

graph::SocketValue read_socket_value(Reader& reader) {
    const auto f32 = [&](std::string_view field) {
        const double value = reader.f64(field);
        if (value < -std::numeric_limits<float>::max() ||
            value > std::numeric_limits<float>::max()) {
            throw TextureDocumentIoError(std::string(field) + " exceeds float range");
        }
        return static_cast<float>(value);
    };
    const std::uint8_t type = reader.u8("socket value type");
    switch (type) {
        case 0:
            return std::monostate{};
        case 1:
            return read_bool(reader, "socket boolean");
        case 2:
            return reader.f64("socket scalar");
        case 3:
            return graph::VectorValue{f32("socket vector x"), f32("socket vector y"),
                                      f32("socket vector z")};
        case 4:
            return graph::ColourValue{f32("socket colour r"), f32("socket colour g"),
                                      f32("socket colour b"), f32("socket colour a")};
        case 5:
            return reader.string("socket string");
        case 6:
            return graph::ImageValue{reader.string("socket image resource")};
        default:
            throw TextureDocumentIoError("socket value type is invalid");
    }
}

doc::AppliedPresetApplication read_application(Reader& reader, DecodeContext& context) {
    doc::AppliedPresetApplication application;
    application.identifier = reader.string("preset application identity");
    application.kind = read_enum<doc::AppliedPresetKind>(reader, 1, "preset application kind");
    application.target_entry_identifier = reader.string("preset application target");
    application.origin.preset_identifier = reader.string("preset origin identity");
    application.origin.schema_version = reader.u32("preset origin schema");
    const auto fragment = reader.blob("preset application fragment");
    application.fragment = doc::deserialize_smart_material(
        {reinterpret_cast<const char*>(fragment.data()), fragment.size()});
    for (const doc::SmartMaterialResourceReference& resource :
         application.fragment.resource_references) {
        context.used_resources.insert(resource.identifier);
    }
    for (const doc::SmartMaterialEntry& entry : application.fragment.stack) {
        if (entry.graph) collect_graph_resources(*entry.graph, context.used_resources);
    }
    const std::uint32_t parameter_count = reader.u32("preset parameter value count");
    require_count(parameter_count, context.limits.maximum_parameter_values,
                  context.totals.parameter_values, "preset parameter value count");
    reader.require_records(parameter_count, 5, "preset parameter value count");
    application.parameter_values.reserve(parameter_count);
    std::set<std::string> parameters;
    for (std::uint32_t index = 0; index < parameter_count; ++index) {
        std::string identity = reader.string("preset parameter identity");
        if (!parameters.insert(identity).second) {
            throw TextureDocumentIoError("preset application repeats parameter identity: " +
                                         identity);
        }
        graph::SocketValue value = read_socket_value(reader);
        collect_socket_resource(value, context.used_resources);
        application.parameter_values.push_back(
            {.parameter_identifier = std::move(identity), .value = std::move(value)});
    }
    return application;
}

void validate_applications(const doc::TextureSet& texture_set,
                           std::span<const doc::AppliedPresetApplication> applications) {
    std::set<std::string_view, std::less<>> application_ids;
    std::set<std::string_view, std::less<>> entry_ids;
    for (const doc::AppliedPresetApplication& application : applications) {
        if (application.identifier.empty() ||
            !application_ids.insert(application.identifier).second) {
            throw TextureDocumentIoError("preset application identity is empty or duplicated");
        }
        if (application.origin.preset_identifier.empty() ||
            application.origin.schema_version == 0) {
            throw TextureDocumentIoError("preset application origin is invalid");
        }
        for (const doc::SmartMaterialEntry& entry : application.fragment.stack) {
            if (!entry_ids.insert(entry.identifier).second ||
                !texture_set.layer_stack().contains(entry.identifier)) {
                throw TextureDocumentIoError(
                    "preset application entry is duplicated or missing from its layer stack");
            }
        }
        for (const doc::AppliedPresetParameterValue& parameter : application.parameter_values) {
            const auto found = std::ranges::find_if(
                application.fragment.exposed_parameters,
                [&](const doc::ExposedSmartMaterialParameter& exposed) {
                    return exposed.identifier == parameter.parameter_identifier;
                });
            if (found == application.fragment.exposed_parameters.end()) {
                throw TextureDocumentIoError(
                    "preset application parameter state names an unknown parameter");
            }
        }
    }
}

void read_texture_set(Reader& reader, doc::TextureDocument& document, DecodeContext& context,
                      std::pmr::memory_resource* memory_resource, TextureDocumentAssetInfo& info) {
    const std::string archived_identity = reader.string("texture-set identity");
    doc::TextureSetDescriptor descriptor{
        .display_name = reader.string("texture-set display name"),
        .partition_kind =
            read_enum<doc::PartitionSourceKind>(reader, 3, "texture-set partition kind"),
        .partition_key = {},
        .uv_set = {},
        .width = 0,
        .height = 0,
        .default_bit_depth = reader.u8("texture-set default bit depth"),
        .udim_tiling = read_bool(reader, "texture-set UDIM flag"),
    };
    static_cast<void>(reader.u8("texture-set reserved byte"));
    descriptor.partition_key = reader.string("texture-set partition key");
    descriptor.uv_set = reader.string("texture-set UV set");
    descriptor.width = reader.u32("texture-set width");
    descriptor.height = reader.u32("texture-set height");
    doc::TextureSet& texture_set = document.create_texture_set(std::move(descriptor));
    if (texture_set.id() != archived_identity) {
        throw TextureDocumentIoError("texture-set stable identity does not match its descriptor");
    }
    restore_pixels(reader, texture_set.channels(), context, true, memory_resource);

    const std::uint32_t udim_count = reader.u32("UDIM tile count");
    require_count(udim_count, context.limits.maximum_udim_tiles, context.totals.udim_tiles,
                  "UDIM tile count");
    std::set<std::uint32_t> udim_numbers;
    for (std::uint32_t index = 0; index < udim_count; ++index) {
        const std::uint32_t number = reader.u32("UDIM tile number");
        if (!udim_numbers.insert(number).second) {
            throw TextureDocumentIoError("texture set repeats a UDIM tile number");
        }
        static_cast<void>(texture_set.ensure_udim_tiles(std::span(&number, 1)));
        restore_pixels(reader, texture_set.udim_channels(number), context, false, memory_resource);
    }

    const std::uint32_t layer_count = reader.u32("layer entry count");
    require_count(layer_count, context.limits.maximum_layer_entries, context.totals.layer_entries,
                  "layer entry count");
    reader.require_records(layer_count, 44, "layer entry count");
    std::vector<doc::LayerEntry> layers;
    layers.reserve(layer_count);
    for (std::uint32_t index = 0; index < layer_count; ++index) {
        layers.push_back(read_layer_entry(reader, context));
    }
    texture_set.layer_stack().assign(std::move(layers));
    info.layer_entry_count += layer_count;

    const std::uint32_t application_count = reader.u32("preset application count");
    require_count(application_count, context.limits.maximum_applications,
                  context.totals.applications, "preset application count");
    reader.require_records(application_count, 33, "preset application count");
    std::vector<doc::AppliedPresetApplication> applications;
    applications.reserve(application_count);
    for (std::uint32_t index = 0; index < application_count; ++index) {
        applications.push_back(read_application(reader, context));
    }
    validate_applications(texture_set, applications);
    TextureDocumentArchiveAccess::restore_applications(texture_set, std::move(applications));
    info.preset_application_count += application_count;

    const auto editable = reader.blob("editable-authoring store");
    texture_set.editable_authoring() = deserialize_editable_authoring(editable);
    info.editable_entry_count += texture_set.editable_authoring().entries().size();
}

doc::AtlasDescriptor read_atlas(Reader& reader, DecodeContext& context) {
    doc::AtlasDescriptor atlas{.identifier = reader.string("atlas identity"),
                               .display_name = reader.string("atlas display name"),
                               .width = reader.u32("atlas width"),
                               .height = reader.u32("atlas height"),
                               .regions = {}};
    const std::uint32_t region_count = reader.u32("atlas region count");
    require_count(region_count, context.limits.maximum_atlas_regions, context.totals.atlas_regions,
                  "atlas region count");
    reader.require_records(region_count, 24, "atlas region count");
    atlas.regions.reserve(region_count);
    for (std::uint32_t index = 0; index < region_count; ++index) {
        atlas.regions.push_back({.texture_set_identifier = reader.string("atlas texture set"),
                                 .x = reader.u32("atlas region x"),
                                 .y = reader.u32("atlas region y"),
                                 .width = reader.u32("atlas region width"),
                                 .height = reader.u32("atlas region height")});
    }
    return atlas;
}

struct DecodedDocument {
    doc::TextureDocument document;
    TextureDocumentAssetInfo info;
};

DecodedDocument decode_document(const ProjectContainer& project, const StandaloneAsset& asset,
                                const TextureDocumentReadLimits& limits,
                                std::pmr::memory_resource* memory_resource) {
    if (asset.kind != texture_document_asset_kind ||
        asset.format_version != current_texture_document_schema) {
        throw TextureDocumentIoError("asset is not a supported texture document: " +
                                     asset.identifier);
    }
    DecodeContext context{.project = project, .asset = asset, .limits = limits};
    for (const StoredTiledImage& image : project.tiled_images) {
        if (!context.images.emplace(image.resource_id, &image).second) {
            throw TextureDocumentIoError("project repeats tiled image identity: " +
                                         image.resource_id);
        }
    }
    for (const std::string& identity : asset.tiled_image_dependencies) {
        if (!context.declared_images.insert(identity).second) {
            throw TextureDocumentIoError("texture-document asset repeats a tiled dependency");
        }
    }
    for (const std::string& identity : asset.resource_dependencies) {
        if (!context.declared_resources.insert(identity).second) {
            throw TextureDocumentIoError("texture-document asset repeats a resource dependency");
        }
        if (std::ranges::none_of(project.resources, [&](const ProjectResource& resource) {
                return resource.identifier == identity;
            })) {
            throw TextureDocumentIoError(
                "texture-document resource dependency has no project metadata: " + identity);
        }
    }
    Reader reader(asset.payload, limits);
    if (!std::ranges::equal(reader.take(document_magic.size(), "texture-document magic"),
                            document_magic)) {
        throw TextureDocumentIoError("texture-document magic is invalid");
    }
    const std::uint32_t schema = reader.u32("texture-document schema");
    if (schema != current_texture_document_schema || schema != asset.format_version) {
        throw TextureDocumentIoError("texture-document schema is unsupported");
    }
    const std::uint32_t texture_set_count = reader.u32("texture-set count");
    const std::uint32_t atlas_count = reader.u32("atlas count");
    std::size_t texture_set_total = 0;
    std::size_t atlas_total = 0;
    require_count(texture_set_count, limits.maximum_texture_sets, texture_set_total,
                  "texture-set count");
    require_count(atlas_count, limits.maximum_atlases, atlas_total, "atlas count");
    reader.require_records(texture_set_count, 48, "texture-set count");
    DecodedDocument decoded{.document = doc::TextureDocument(memory_resource),
                            .info = {.identifier = asset.identifier,
                                     .texture_set_count = texture_set_count,
                                     .tiled_image_count = asset.tiled_image_dependencies.size(),
                                     .atlas_count = atlas_count}};
    for (std::uint32_t index = 0; index < texture_set_count; ++index) {
        read_texture_set(reader, decoded.document, context, memory_resource, decoded.info);
    }
    for (std::uint32_t index = 0; index < atlas_count; ++index) {
        static_cast<void>(decoded.document.create_atlas(read_atlas(reader, context)));
    }
    if (!reader.empty()) {
        throw TextureDocumentIoError("texture-document payload has trailing bytes");
    }
    if (context.used_images.size() != context.declared_images.size() ||
        std::ranges::any_of(context.declared_images, [&](std::string_view identity) {
            return !context.used_images.contains(identity);
        })) {
        throw TextureDocumentIoError(
            "texture-document tiled dependencies do not exactly match the payload");
    }
    if (context.used_resources.size() != context.declared_resources.size() ||
        std::ranges::any_of(context.declared_resources, [&](std::string_view identity) {
            return !context.used_resources.contains(identity);
        })) {
        throw TextureDocumentIoError(
            "texture-document resource dependencies do not exactly match the payload");
    }
    return decoded;
}

const StandaloneAsset& require_document_asset(const ProjectContainer& project,
                                              std::string_view identifier) {
    const auto found = std::ranges::find_if(project.assets, [&](const StandaloneAsset& asset) {
        return asset.identifier == identifier;
    });
    if (found == project.assets.end()) {
        throw TextureDocumentIoError("texture-document asset does not exist: " +
                                     std::string(identifier));
    }
    return *found;
}

bool asset_uses_image(const StandaloneAsset& asset, std::string_view identity) {
    return std::ranges::find(asset.tiled_image_dependencies, identity) !=
           asset.tiled_image_dependencies.end();
}

void remove_previous_document(ProjectContainer& candidate, std::string_view identifier) {
    const auto asset = std::ranges::find_if(candidate.assets, [&](const StandaloneAsset& current) {
        return current.identifier == identifier;
    });
    if (asset == candidate.assets.end()) return;
    if (asset->kind != texture_document_asset_kind) {
        throw TextureDocumentIoError("asset identity belongs to a different kind: " +
                                     std::string(identifier));
    }
    const std::vector<std::string> dependencies = asset->tiled_image_dependencies;
    candidate.assets.erase(asset);
    for (const std::string& dependency : dependencies) {
        if (std::ranges::any_of(candidate.assets, [&](const StandaloneAsset& other) {
                return asset_uses_image(other, dependency);
            })) {
            throw TextureDocumentIoError(
                "texture-document pixel dependency is shared by another asset: " + dependency);
        }
        std::erase_if(candidate.tiled_images, [&](const StoredTiledImage& image) {
            return image.resource_id == dependency;
        });
    }
}

void require_known_resources(const ProjectContainer& project, const StandaloneAsset& asset) {
    for (const std::string& dependency : asset.resource_dependencies) {
        if (std::ranges::none_of(project.resources, [&](const ProjectResource& resource) {
                return resource.identifier == dependency;
            })) {
            throw TextureDocumentIoError(
                "texture document references resource without project "
                "metadata: " +
                dependency);
        }
    }
}

}  // namespace

std::vector<TextureDocumentAssetInfo> list_texture_documents(const ProjectContainer& project,
                                                             TextureDocumentReadLimits limits) {
    std::vector<TextureDocumentAssetInfo> result;
    for (const StandaloneAsset& asset : project.assets) {
        if (asset.kind != texture_document_asset_kind) continue;
        result.push_back(
            decode_document(project, asset, limits, std::pmr::get_default_resource()).info);
    }
    std::ranges::sort(result, {}, &TextureDocumentAssetInfo::identifier);
    return result;
}

void upsert_texture_document(ProjectContainer& project, std::string identifier,
                             const doc::TextureDocument& document) {
    PackagedDocument packaged = package_document(std::move(identifier), document);
    ProjectContainer candidate = project;
    remove_previous_document(candidate, packaged.asset.identifier);
    require_known_resources(candidate, packaged.asset);
    for (const StoredTiledImage& image : packaged.images) {
        if (std::ranges::any_of(candidate.tiled_images, [&](const StoredTiledImage& existing) {
                return existing.resource_id == image.resource_id;
            })) {
            throw TextureDocumentIoError("texture-document tiled image identity collides: " +
                                         image.resource_id);
        }
        candidate.tiled_images.push_back(image);
    }
    candidate.assets.push_back(std::move(packaged.asset));
    std::ranges::sort(candidate.tiled_images, {}, &StoredTiledImage::resource_id);
    std::ranges::sort(candidate.assets, {}, &StandaloneAsset::identifier);
    static_cast<void>(write_project_container(candidate));
    project = std::move(candidate);
}

doc::TextureDocument unpack_texture_document(const ProjectContainer& project,
                                             std::string_view identifier,
                                             TextureDocumentReadLimits limits,
                                             std::pmr::memory_resource* memory_resource) {
    if (memory_resource == nullptr) {
        throw TextureDocumentIoError("texture-document decoder requires a memory resource");
    }
    return decode_document(project, require_document_asset(project, identifier), limits,
                           memory_resource)
        .document;
}

}  // namespace ctex::io
