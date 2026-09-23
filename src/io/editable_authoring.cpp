#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <ctex/io/editable_authoring.hpp>
#include <limits>
#include <string>
#include <utility>

namespace ctex::io {
namespace {

constexpr std::array<std::byte, 8> editable_magic{std::byte{'C'}, std::byte{'T'}, std::byte{'E'},
                                                  std::byte{'X'}, std::byte{'E'}, std::byte{'D'},
                                                  std::byte{'T'}, std::byte{0}};

class Writer {
public:
    void u8(std::uint8_t value) { bytes_.push_back(static_cast<std::byte>(value)); }
    void u32(std::uint32_t value) {
        for (unsigned shift = 0; shift < 32; shift += 8) {
            u8(static_cast<std::uint8_t>(value >> shift));
        }
    }
    void u64(std::uint64_t value) {
        for (unsigned shift = 0; shift < 64; shift += 8) {
            u8(static_cast<std::uint8_t>(value >> shift));
        }
    }
    void f64(double value) { u64(std::bit_cast<std::uint64_t>(value)); }
    void bytes(std::span<const std::byte> value) {
        bytes_.insert(bytes_.end(), value.begin(), value.end());
    }
    void string(std::string_view value) {
        if (value.size() > std::numeric_limits<std::uint32_t>::max()) {
            throw EditableAuthoringIoError("editable-authoring string exceeds format limit");
        }
        u32(static_cast<std::uint32_t>(value.size()));
        bytes(std::as_bytes(std::span(value)));
    }
    [[nodiscard]] std::vector<std::byte> finish() && { return std::move(bytes_); }

private:
    std::vector<std::byte> bytes_;
};

class Reader {
public:
    Reader(std::span<const std::byte> bytes, const EditableAuthoringReadLimits& limits)
        : bytes_(bytes), limits_(limits) {}

    [[nodiscard]] std::span<const std::byte> take(std::size_t size, std::string_view field) {
        if (size > bytes_.size() - offset_) {
            throw EditableAuthoringIoError("editable-authoring ends inside " + std::string(field));
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
        const double result = std::bit_cast<double>(u64(field));
        if (!std::isfinite(result)) {
            throw EditableAuthoringIoError("editable-authoring contains non-finite " +
                                           std::string(field));
        }
        return result;
    }
    [[nodiscard]] std::string string(std::string_view field) {
        const std::size_t size = u32(field);
        if (size > limits_.maximum_string_bytes) {
            throw EditableAuthoringIoError("editable-authoring string exceeds configured limit");
        }
        const auto value = take(size, field);
        return {reinterpret_cast<const char*>(value.data()), value.size()};
    }
    [[nodiscard]] bool empty() const noexcept { return offset_ == bytes_.size(); }
    [[nodiscard]] std::size_t remaining() const noexcept { return bytes_.size() - offset_; }

private:
    std::span<const std::byte> bytes_;
    const EditableAuthoringReadLimits& limits_;
    std::size_t offset_{};
};

void check_count(std::size_t count, std::size_t limit, std::string_view field) {
    if (count > limit || count > std::numeric_limits<std::uint32_t>::max()) {
        throw EditableAuthoringIoError(std::string(field) + " exceeds configured limit");
    }
}

void consume_count(std::size_t count, std::size_t& total, std::size_t limit,
                   std::string_view field) {
    if (count > limit - std::min(total, limit)) {
        throw EditableAuthoringIoError(std::string(field) + " exceeds configured aggregate limit");
    }
    total += count;
}

std::size_t checked_minimum_bytes(std::size_t count, std::size_t item_size,
                                  std::string_view field) {
    if (count > std::numeric_limits<std::size_t>::max() / item_size) {
        throw EditableAuthoringIoError(std::string(field) + " byte count overflows");
    }
    return count * item_size;
}

struct ReadTotals {
    std::size_t parameters{};
    std::size_t surface_points{};
    std::size_t tile_dependencies{};
};

template <std::size_t Size>
void write_array(Writer& writer, const std::array<double, Size>& values) {
    for (double value : values) writer.f64(value);
}

template <std::size_t Size>
std::array<double, Size> read_array(Reader& reader, std::string_view field) {
    std::array<double, Size> result;
    for (double& value : result) value = reader.f64(field);
    return result;
}

void write_entry(Writer& writer, const doc::EditableAuthoringEntry& entry) {
    writer.string(entry.identifier);
    writer.u8(static_cast<std::uint8_t>(entry.kind));
    writer.u8(0);
    writer.u8(0);
    writer.u8(0);
    writer.u64(entry.revision);
    write_array(writer, entry.placement.position);
    write_array(writer, entry.placement.normal);
    writer.f64(entry.placement.rotation_radians);
    writer.f64(entry.placement.uniform_scale);
    write_array(writer, entry.placement.axis_scale);
    writer.string(entry.material_identity);
    writer.string(entry.text);
    writer.string(entry.font_identity);
    writer.u64(entry.mesh_revision);
    writer.u32(static_cast<std::uint32_t>(entry.material_parameters.size()));
    writer.u32(static_cast<std::uint32_t>(entry.surface_points.size()));
    writer.u32(static_cast<std::uint32_t>(entry.dependent_tiles.size()));
    for (const doc::EditableMaterialParameter& parameter : entry.material_parameters) {
        writer.string(parameter.identifier);
        writer.u8(parameter.component_count);
        writer.u8(0);
        writer.u8(0);
        writer.u8(0);
        write_array(writer, parameter.value);
    }
    for (const doc::EditableSurfacePoint& point : entry.surface_points) {
        write_array(writer, point.position);
        write_array(writer, point.normal);
        writer.u32(point.triangle);
        write_array(writer, point.barycentric);
        writer.f64(point.width);
    }
    for (const doc::EditableTileDependency& tile : entry.dependent_tiles) {
        writer.string(tile.semantic_id);
        writer.u32(tile.tile_x);
        writer.u32(tile.tile_y);
    }
}

doc::EditableAuthoringEntry read_entry(Reader& reader, const EditableAuthoringReadLimits& limits,
                                       ReadTotals& totals) {
    doc::EditableAuthoringEntry entry;
    entry.identifier = reader.string("entry identity");
    entry.kind = static_cast<doc::EditableEntryKind>(reader.u8("entry kind"));
    static_cast<void>(reader.take(3, "entry reserved bytes"));
    entry.revision = reader.u64("entry revision");
    entry.placement.position = read_array<3>(reader, "placement position");
    entry.placement.normal = read_array<3>(reader, "placement normal");
    entry.placement.rotation_radians = reader.f64("placement rotation");
    entry.placement.uniform_scale = reader.f64("placement scale");
    entry.placement.axis_scale = read_array<2>(reader, "placement axis scale");
    entry.material_identity = reader.string("material identity");
    entry.text = reader.string("text");
    entry.font_identity = reader.string("font identity");
    entry.mesh_revision = reader.u64("mesh revision");
    const std::uint32_t parameter_count = reader.u32("parameter count");
    const std::uint32_t point_count = reader.u32("surface point count");
    const std::uint32_t tile_count = reader.u32("tile dependency count");
    check_count(parameter_count, limits.maximum_parameters, "parameter count");
    check_count(point_count, limits.maximum_surface_points, "surface point count");
    check_count(tile_count, limits.maximum_tile_dependencies, "tile dependency count");
    consume_count(parameter_count, totals.parameters, limits.maximum_parameters, "parameter count");
    consume_count(point_count, totals.surface_points, limits.maximum_surface_points,
                  "surface point count");
    consume_count(tile_count, totals.tile_dependencies, limits.maximum_tile_dependencies,
                  "tile dependency count");
    const std::size_t parameter_bytes =
        checked_minimum_bytes(parameter_count, 40, "parameter count");
    const std::size_t point_bytes = checked_minimum_bytes(point_count, 84, "surface point count");
    const std::size_t tile_bytes = checked_minimum_bytes(tile_count, 12, "tile dependency count");
    if (parameter_bytes > reader.remaining() ||
        point_bytes > reader.remaining() - parameter_bytes ||
        tile_bytes > reader.remaining() - parameter_bytes - point_bytes) {
        throw EditableAuthoringIoError(
            "editable-authoring collection counts exceed the remaining input");
    }
    entry.material_parameters.reserve(parameter_count);
    for (std::uint32_t index = 0; index < parameter_count; ++index) {
        doc::EditableMaterialParameter parameter;
        parameter.identifier = reader.string("parameter identity");
        parameter.component_count = reader.u8("parameter component count");
        static_cast<void>(reader.take(3, "parameter reserved bytes"));
        parameter.value = read_array<4>(reader, "parameter value");
        entry.material_parameters.push_back(std::move(parameter));
    }
    entry.surface_points.reserve(point_count);
    for (std::uint32_t index = 0; index < point_count; ++index) {
        entry.surface_points.push_back({.position = read_array<3>(reader, "point position"),
                                        .normal = read_array<3>(reader, "point normal"),
                                        .triangle = reader.u32("point triangle"),
                                        .barycentric = read_array<3>(reader, "point barycentric"),
                                        .width = reader.f64("point width")});
    }
    entry.dependent_tiles.reserve(tile_count);
    for (std::uint32_t index = 0; index < tile_count; ++index) {
        entry.dependent_tiles.push_back({.semantic_id = reader.string("tile semantic identity"),
                                         .tile_x = reader.u32("tile x"),
                                         .tile_y = reader.u32("tile y")});
    }
    return entry;
}

}  // namespace

std::vector<std::byte> serialize_editable_authoring(const doc::EditableAuthoringStore& store) {
    check_count(store.entries().size(), std::numeric_limits<std::uint32_t>::max(), "entry count");
    Writer writer;
    writer.bytes(editable_magic);
    writer.u32(current_editable_authoring_schema);
    writer.u32(static_cast<std::uint32_t>(store.entries().size()));
    for (const doc::EditableAuthoringEntry& entry : store.entries()) {
        doc::validate_editable_authoring_entry(entry);
        check_count(entry.material_parameters.size(), std::numeric_limits<std::uint32_t>::max(),
                    "parameter count");
        check_count(entry.surface_points.size(), std::numeric_limits<std::uint32_t>::max(),
                    "surface point count");
        check_count(entry.dependent_tiles.size(), std::numeric_limits<std::uint32_t>::max(),
                    "tile dependency count");
        write_entry(writer, entry);
    }
    return std::move(writer).finish();
}

doc::EditableAuthoringStore deserialize_editable_authoring(std::span<const std::byte> serialized,
                                                           EditableAuthoringReadLimits limits) {
    if (serialized.size() > limits.maximum_input_bytes) {
        throw EditableAuthoringIoError("editable-authoring exceeds configured input limit");
    }
    Reader reader(serialized, limits);
    if (!std::ranges::equal(reader.take(editable_magic.size(), "magic"), editable_magic)) {
        throw EditableAuthoringIoError("editable-authoring magic is invalid");
    }
    if (reader.u32("schema") != current_editable_authoring_schema) {
        throw EditableAuthoringIoError("editable-authoring schema is unsupported");
    }
    const std::uint32_t count = reader.u32("entry count");
    check_count(count, limits.maximum_entries, "entry count");
    std::vector<doc::EditableAuthoringEntry> entries;
    entries.reserve(count);
    ReadTotals totals;
    for (std::uint32_t index = 0; index < count; ++index) {
        entries.push_back(read_entry(reader, limits, totals));
    }
    if (!reader.empty()) {
        throw EditableAuthoringIoError("editable-authoring contains trailing bytes");
    }
    try {
        return doc::EditableAuthoringStore(std::move(entries));
    } catch (const doc::EditableAuthoringError& error) {
        throw EditableAuthoringIoError(error.what());
    }
}

StandaloneAsset package_editable_authoring(std::string identifier,
                                           const doc::EditableAuthoringStore& store) {
    if (identifier.empty()) {
        throw EditableAuthoringIoError("editable-authoring asset identity is empty");
    }
    return {.identifier = std::move(identifier),
            .kind = std::string(editable_authoring_asset_kind),
            .format_version = current_editable_authoring_schema,
            .resource_dependencies = {},
            .tiled_image_dependencies = {},
            .payload = serialize_editable_authoring(store)};
}

doc::EditableAuthoringStore unpack_editable_authoring(const StandaloneAsset& asset,
                                                      EditableAuthoringReadLimits limits) {
    if (asset.kind != editable_authoring_asset_kind ||
        asset.format_version != current_editable_authoring_schema) {
        throw EditableAuthoringIoError("asset is not supported editable-authoring data");
    }
    return deserialize_editable_authoring(asset.payload, limits);
}

void upsert_editable_authoring(ProjectContainer& project, std::string identifier,
                               const doc::EditableAuthoringStore& store) {
    StandaloneAsset replacement = package_editable_authoring(std::move(identifier), store);
    const auto found =
        std::ranges::find(project.assets, replacement.identifier, &StandaloneAsset::identifier);
    if (found == project.assets.end()) {
        project.assets.push_back(std::move(replacement));
    } else {
        if (found->kind != editable_authoring_asset_kind) {
            throw EditableAuthoringIoError(
                "editable-authoring asset identity conflicts with another asset kind");
        }
        *found = std::move(replacement);
    }
}

}  // namespace ctex::io
