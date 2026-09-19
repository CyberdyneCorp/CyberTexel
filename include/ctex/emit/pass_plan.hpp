#ifndef CTEX_EMIT_PASS_PLAN_HPP
#define CTEX_EMIT_PASS_PLAN_HPP

#include <compare>
#include <cstdint>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace ctex::emit {

struct ResourceVersion {
    std::string logical_id;
    std::uint64_t generation{};
    friend bool operator==(const ResourceVersion&, const ResourceVersion&) = default;
};

enum class TextureFormat : std::uint8_t {
    r8_unorm,
    rg8_unorm,
    rgba8_unorm,
    r16_unorm,
    rg16_unorm,
    rgba16_unorm,
    r16_float,
    rg16_float,
    rgba16_float,
    r32_float,
    rg32_float,
    rgba32_float,
    depth32_float,
};

struct TextureExtent {
    std::uint32_t width{};
    std::uint32_t height{};
    std::uint32_t layers{1};
    friend bool operator==(TextureExtent, TextureExtent) noexcept = default;
};

struct TileShape {
    std::uint32_t width{};
    std::uint32_t height{};
    friend bool operator==(TileShape, TileShape) noexcept = default;
};

struct LogicalTexture {
    ResourceVersion version;
    std::string role;
    TextureFormat format{};
    TextureExtent extent;
    std::uint32_t mip_levels{1};
    TileShape tile_shape;
    bool externally_initialized{};
    friend bool operator==(const LogicalTexture&, const LogicalTexture&) = default;
};

struct TileRange {
    std::uint32_t x{};
    std::uint32_t y{};
    std::uint32_t width{};
    std::uint32_t height{};
    friend bool operator==(TileRange, TileRange) noexcept = default;
};

struct TextureSubresourceRange {
    std::uint32_t first_mip{};
    std::uint32_t mip_count{1};
    std::uint32_t first_layer{};
    std::uint32_t layer_count{1};
    std::optional<TileRange> tiles;
    friend bool operator==(const TextureSubresourceRange&,
                           const TextureSubresourceRange&) = default;
};

enum class ResourceAccessMode : std::uint8_t { read, write, read_write };
enum class LoadAction : std::uint8_t { load, clear, discard };
enum class StoreAction : std::uint8_t { not_applicable, store, discard };

struct ResourceAccess {
    ResourceVersion resource;
    TextureSubresourceRange subresources;
    ResourceAccessMode mode{};
    LoadAction load{};
    StoreAction store{};
    friend bool operator==(const ResourceAccess&, const ResourceAccess&) = default;
};

enum class ShaderVisibility : std::uint8_t { vertex, fragment, vertex_fragment, compute };
enum class FilterMode : std::uint8_t { nearest, linear };
enum class AddressMode : std::uint8_t { clamp_to_edge, repeat, mirror_repeat };
enum class TextureBindingKind : std::uint8_t {
    sampled,
    storage_read,
    storage_write,
    storage_read_write,
};
enum class TextureViewDimension : std::uint8_t { d2, cube };

struct TextureBinding {
    std::uint32_t group{};
    std::uint32_t binding{};
    std::string role;
    ShaderVisibility visibility{};
    TextureBindingKind kind{};
    ResourceVersion resource;
    TextureSubresourceRange subresources;
    TextureViewDimension view_dimension{TextureViewDimension::d2};
    std::string encoding{};
    std::string mip_convention{};
    friend bool operator==(const TextureBinding&, const TextureBinding&) = default;
};

struct SamplerBinding {
    std::uint32_t group{};
    std::uint32_t binding{};
    std::string role;
    ShaderVisibility visibility{};
    FilterMode min_filter{};
    FilterMode mag_filter{};
    FilterMode mip_filter{};
    AddressMode address_u{};
    AddressMode address_v{};
    AddressMode address_w{};
    friend bool operator==(const SamplerBinding&, const SamplerBinding&) = default;
};

struct UniformField {
    std::string name;
    std::uint32_t offset{};
    std::uint32_t size{};
    std::uint32_t alignment{};
    friend bool operator==(const UniformField&, const UniformField&) = default;
};

struct UniformBlockLayout {
    std::uint32_t group{};
    std::uint32_t binding{};
    std::string role;
    ShaderVisibility visibility{};
    std::uint32_t size{};
    std::vector<UniformField> fields;
    friend bool operator==(const UniformBlockLayout&, const UniformBlockLayout&) = default;
};

enum class VertexFormat : std::uint8_t { float32, float32x2, float32x3, float32x4, uint32 };
enum class VertexStepMode : std::uint8_t { vertex, instance };

struct VertexAttribute {
    std::string semantic;
    std::uint32_t location{};
    VertexFormat format{};
    std::uint32_t offset{};
    friend bool operator==(const VertexAttribute&, const VertexAttribute&) = default;
};

struct VertexBufferLayout {
    std::uint32_t slot{};
    std::uint32_t stride{};
    VertexStepMode step_mode{};
    std::vector<VertexAttribute> attributes;
    friend bool operator==(const VertexBufferLayout&, const VertexBufferLayout&) = default;
};

enum class CompareFunction : std::uint8_t {
    never,
    less,
    less_equal,
    equal,
    greater_equal,
    greater,
    not_equal,
    always,
};

struct DepthState {
    bool test_enabled{};
    bool write_enabled{};
    CompareFunction compare{CompareFunction::always};
    friend bool operator==(DepthState, DepthState) noexcept = default;
};

enum class BlendFactor : std::uint8_t {
    zero,
    one,
    source,
    one_minus_source,
    source_alpha,
    one_minus_source_alpha,
    destination,
    one_minus_destination,
    destination_alpha,
    one_minus_destination_alpha,
};
enum class BlendOperation : std::uint8_t { add, subtract, reverse_subtract, minimum, maximum };

struct BlendComponent {
    BlendFactor source{BlendFactor::one};
    BlendFactor destination{BlendFactor::zero};
    BlendOperation operation{BlendOperation::add};
    friend bool operator==(BlendComponent, BlendComponent) noexcept = default;
};

struct BlendState {
    bool enabled{};
    BlendComponent colour;
    BlendComponent alpha;
    std::uint8_t write_mask{0x0f};
    friend bool operator==(BlendState, BlendState) noexcept = default;
};

struct ClearColour {
    float r{};
    float g{};
    float b{};
    float a{};
    friend bool operator==(ClearColour, ClearColour) noexcept = default;
};

struct RenderTarget {
    std::string role;
    ResourceVersion resource;
    TextureSubresourceRange subresources;
    TextureFormat format{};
    std::uint32_t width{};
    std::uint32_t height{};
    LoadAction load{};
    StoreAction store{};
    ClearColour clear_colour;
    BlendState blend;
    friend bool operator==(const RenderTarget&, const RenderTarget&) = default;
};

struct DepthTarget {
    std::string role;
    ResourceVersion resource;
    TextureSubresourceRange subresources;
    TextureFormat format{TextureFormat::depth32_float};
    std::uint32_t width{};
    std::uint32_t height{};
    LoadAction load{};
    StoreAction store{};
    float clear_depth{1.0F};
    friend bool operator==(const DepthTarget&, const DepthTarget&) = default;
};

enum class PrimitiveTopology : std::uint8_t {
    point_list,
    line_list,
    line_strip,
    triangle_list,
    triangle_strip,
};

struct DrawCommand {
    PrimitiveTopology topology{PrimitiveTopology::triangle_list};
    bool indexed{};
    std::uint32_t vertex_count{};
    std::uint32_t index_count{};
    std::uint32_t instance_count{1};
    std::uint32_t first_vertex{};
    std::uint32_t first_index{};
    std::int32_t base_vertex{};
    std::uint32_t first_instance{};
    friend bool operator==(DrawCommand, DrawCommand) noexcept = default;
};

struct DispatchCommand {
    std::uint32_t x{};
    std::uint32_t y{};
    std::uint32_t z{};
    friend bool operator==(DispatchCommand, DispatchCommand) noexcept = default;
};

using PassCommand = std::variant<DrawCommand, DispatchCommand>;
enum class PassKind : std::uint8_t { render, compute };

enum class HostCacheResourceKind : std::uint8_t { render_pipeline, compute_pipeline };

struct HostCacheIdentity {
    HostCacheResourceKind kind{};
    std::string scope;
    std::string resource;

    friend bool operator==(const HostCacheIdentity&, const HostCacheIdentity&) = default;
    friend auto operator<=>(const HostCacheIdentity&, const HostCacheIdentity&) = default;
};

struct PassDescriptor {
    std::string identifier;
    PassKind kind{};
    std::vector<std::string> dependencies;
    std::string vertex_entry_point;
    std::string fragment_entry_point;
    std::string compute_entry_point;
    std::vector<ResourceAccess> accesses;
    std::vector<TextureBinding> texture_bindings;
    std::vector<SamplerBinding> sampler_bindings;
    std::vector<UniformBlockLayout> uniform_blocks;
    std::vector<VertexBufferLayout> vertex_buffers;
    std::vector<RenderTarget> render_targets;
    std::optional<DepthTarget> depth_target;
    DepthState depth_state;
    PassCommand command;
    friend bool operator==(const PassDescriptor&, const PassDescriptor&) = default;
};

struct ResourceLifetime {
    ResourceVersion resource;
    std::uint32_t first_pass{};
    std::uint32_t last_pass{};
    friend bool operator==(const ResourceLifetime&, const ResourceLifetime&) = default;
};

class PassPlanError final : public std::invalid_argument {
public:
    using std::invalid_argument::invalid_argument;
};

class PassPlan {
public:
    PassPlan(std::string stable_identity, std::vector<LogicalTexture> resources,
             std::vector<PassDescriptor> passes);

    [[nodiscard]] const std::string& stable_identity() const noexcept { return stable_identity_; }
    [[nodiscard]] std::span<const LogicalTexture> resources() const noexcept { return resources_; }
    [[nodiscard]] std::span<const PassDescriptor> passes() const noexcept { return passes_; }
    [[nodiscard]] std::span<const ResourceLifetime> lifetimes() const noexcept {
        return lifetimes_;
    }
    [[nodiscard]] HostCacheIdentity pipeline_identity(std::string_view pass_identifier) const;

    friend bool operator==(const PassPlan&, const PassPlan&) = default;

private:
    std::string stable_identity_;
    std::vector<LogicalTexture> resources_;
    std::vector<PassDescriptor> passes_;
    std::vector<ResourceLifetime> lifetimes_;
};

}  // namespace ctex::emit

#endif
