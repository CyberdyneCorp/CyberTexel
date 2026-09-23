#include <algorithm>
#include <array>
#include <cmath>
#include <ctex/doc/document.hpp>
#include <ctex/graph/portable_nodes.hpp>
#include <limits>
#include <map>
#include <set>
#include <utility>

namespace ctex::doc {
namespace {

[[noreturn]] void fail(LayerCompositeErrorCode code, std::string message) {
    throw LayerCompositeError(code, std::move(message));
}

std::size_t checked_pixel_count(std::uint32_t width, std::uint32_t height) {
    if (width == 0 || height == 0 ||
        static_cast<std::size_t>(width) >
            std::numeric_limits<std::size_t>::max() / static_cast<std::size_t>(height)) {
        fail(LayerCompositeErrorCode::invalid_request,
             "layer composite dimensions are invalid or exceed addressable storage");
    }
    return static_cast<std::size_t>(width) * height;
}

bool normalized(float value) { return std::isfinite(value) && value >= 0.0F && value <= 1.0F; }

bool normalized(graph::ColourValue value) {
    return normalized(value.r) && normalized(value.g) && normalized(value.b) && normalized(value.a);
}

bool normalized_defaults(const ChannelDescriptor& descriptor) {
    return std::ranges::all_of(descriptor.default_value, [](double value) {
        return std::isfinite(value) && value >= 0.0 && value <= 1.0;
    });
}

using InputKey = std::pair<std::string_view, std::string_view>;
using EntryIndices = std::map<std::string_view, std::size_t, std::less<>>;

bool takes_content_input(LayerEntryKind kind) {
    return kind == LayerEntryKind::paint_layer || kind == LayerEntryKind::fill_layer ||
           kind == LayerEntryKind::filter || kind == LayerEntryKind::editable_decal ||
           kind == LayerEntryKind::editable_text || kind == LayerEntryKind::surface_path;
}

class CompositeInputs {
public:
    CompositeInputs(const TextureSet& texture_set, const LayerCompositeRequest& request,
                    std::size_t pixel_count)
        : request_(request), entries_(texture_set.layer_stack().entries()) {
        for (std::size_t index = 0; index < entries_.size(); ++index) {
            entry_indices_.emplace(entries_[index].identifier, index);
        }
        for (const std::string& semantic_id : texture_set.channels().semantic_ids()) {
            semantic_ids_.insert(semantic_id);
        }
        validate_content(pixel_count);
        validate_masks(pixel_count);
    }

    [[nodiscard]] const LayerCompositeRaster& content(std::string_view entry_identifier,
                                                      std::string_view semantic_id) const {
        const auto found = content_.find({entry_identifier, semantic_id});
        if (found == content_.end()) {
            fail(LayerCompositeErrorCode::missing_content,
                 "layer composite content is missing for entry '" + std::string(entry_identifier) +
                     "' and channel '" + std::string(semantic_id) + "'");
        }
        return *found->second;
    }

    [[nodiscard]] const LayerCompositeMaskRaster& mask(std::string_view identifier) const {
        const auto found = masks_.find(identifier);
        if (found == masks_.end()) {
            fail(LayerCompositeErrorCode::missing_mask,
                 "layer composite mask is missing for entry '" + std::string(identifier) + "'");
        }
        return *found->second;
    }

private:
    void validate_content(std::size_t pixel_count) {
        for (const LayerCompositeRaster& raster : request_.content) {
            const auto entry = entry_indices_.find(raster.entry_identifier);
            if (entry == entry_indices_.end() ||
                !takes_content_input(entries_[entry->second].kind) ||
                !semantic_ids_.contains(raster.semantic_id)) {
                fail(LayerCompositeErrorCode::unknown_input,
                     "layer composite content names an unknown entry, unsupported entry kind, or "
                     "unregistered channel");
            }
            if (raster.width != request_.width || raster.height != request_.height ||
                raster.pixels.size() != pixel_count ||
                (!raster.coverage.empty() && raster.coverage.size() != pixel_count) ||
                !std::ranges::all_of(raster.pixels,
                                     [](graph::ColourValue value) { return normalized(value); }) ||
                !std::ranges::all_of(raster.coverage,
                                     [](float value) { return normalized(value); })) {
                fail(LayerCompositeErrorCode::invalid_request,
                     "layer composite content dimensions, pixels, or coverage are invalid");
            }
            if (!content_.emplace(InputKey{raster.entry_identifier, raster.semantic_id}, &raster)
                     .second) {
                fail(LayerCompositeErrorCode::duplicate_input,
                     "layer composite repeats an entry/channel content input");
            }
        }
    }

    void validate_masks(std::size_t pixel_count) {
        for (const LayerCompositeMaskRaster& raster : request_.masks) {
            const auto entry = entry_indices_.find(raster.mask_identifier);
            if (entry == entry_indices_.end() ||
                entries_[entry->second].kind != LayerEntryKind::mask) {
                fail(LayerCompositeErrorCode::unknown_input,
                     "layer composite mask input does not name a mask entry");
            }
            if (raster.width != request_.width || raster.height != request_.height ||
                raster.values.size() != pixel_count ||
                !std::ranges::all_of(raster.values, [](double value) {
                    return std::isfinite(value) && value >= 0.0 && value <= 1.0;
                })) {
                fail(LayerCompositeErrorCode::invalid_request,
                     "layer composite mask dimensions or values are invalid");
            }
            if (!masks_.emplace(raster.mask_identifier, &raster).second) {
                fail(LayerCompositeErrorCode::duplicate_input,
                     "layer composite repeats a mask input");
            }
        }
    }

    const LayerCompositeRequest& request_;
    std::span<const LayerEntry> entries_;
    EntryIndices entry_indices_;
    std::set<std::string, std::less<>> semantic_ids_;
    std::map<InputKey, const LayerCompositeRaster*, std::less<>> content_;
    std::map<std::string_view, const LayerCompositeMaskRaster*, std::less<>> masks_;
};

struct CompositeSample {
    graph::ColourValue value{};
    float coverage{};
};

using CompositeRaster = std::vector<CompositeSample>;

std::array<float, 4> components(graph::ColourValue value) {
    return {value.r, value.g, value.b, value.a};
}

graph::ColourValue colour(std::array<float, 4> value) {
    return {.r = value[0], .g = value[1], .b = value[2], .a = value[3]};
}

float blend_scalar(std::string_view mode, float base, float layer) {
    const graph::ColourValue repeated_base{base, base, base, base};
    const graph::ColourValue repeated_layer{layer, layer, layer, layer};
    return graph::blend_colour(mode, repeated_base, repeated_layer, 1.0).r;
}

graph::ColourValue unweighted_blend(const ChannelDescriptor& descriptor, std::string_view mode,
                                    graph::ColourValue base, graph::ColourValue layer) {
    if (descriptor.blending_policy == BlendingPolicy::color ||
        descriptor.blending_policy == BlendingPolicy::normal_vector) {
        return graph::blend_colour(mode, base, layer, 1.0);
    }
    std::array<float, 4> output{};
    const std::array<float, 4> base_components = components(base);
    const std::array<float, 4> layer_components = components(layer);
    for (std::size_t component = 0; component < descriptor.component_count; ++component) {
        output[component] =
            descriptor.blending_policy == BlendingPolicy::additive
                ? std::min(1.0F, base_components[component] + layer_components[component])
                : blend_scalar(mode, base_components[component], layer_components[component]);
    }
    return colour(output);
}

graph::ColourValue normalize_vector(graph::ColourValue value) {
    const float x = value.r * 2.0F - 1.0F;
    const float y = value.g * 2.0F - 1.0F;
    const float z = value.b * 2.0F - 1.0F;
    const float length = std::sqrt(x * x + y * y + z * z);
    if (length <= std::numeric_limits<float>::epsilon()) {
        return {.r = 0.5F, .g = 0.5F, .b = 1.0F, .a = value.a};
    }
    return {.r = x / length * 0.5F + 0.5F,
            .g = y / length * 0.5F + 0.5F,
            .b = z / length * 0.5F + 0.5F,
            .a = value.a};
}

CompositeSample composite_sample(const ChannelDescriptor& descriptor, std::string_view mode,
                                 CompositeSample base, CompositeSample layer, double factor) {
    const float source_coverage =
        std::clamp(layer.coverage * static_cast<float>(factor), 0.0F, 1.0F);
    if (source_coverage == 0.0F) {
        return base;
    }
    const float output_coverage = source_coverage + base.coverage * (1.0F - source_coverage);
    const graph::ColourValue blended = unweighted_blend(descriptor, mode, base.value, layer.value);
    const std::array<float, 4> base_values = components(base.value);
    const std::array<float, 4> layer_values = components(layer.value);
    const std::array<float, 4> blend_values = components(blended);
    std::array<float, 4> output{};
    for (std::size_t component = 0; component < descriptor.component_count; ++component) {
        const float premultiplied =
            (1.0F - source_coverage) * base.coverage * base_values[component] +
            (1.0F - base.coverage) * source_coverage * layer_values[component] +
            base.coverage * source_coverage * blend_values[component];
        output[component] = output_coverage == 0.0F ? 0.0F : premultiplied / output_coverage;
    }
    graph::ColourValue result = colour(output);
    if (descriptor.blending_policy == BlendingPolicy::normal_vector) {
        result = normalize_vector(result);
    }
    return {.value = result, .coverage = output_coverage};
}

const LayerEntry::ChannelModulation* channel_modulation(const LayerEntry& entry,
                                                        std::string_view semantic_id) {
    const auto found =
        std::ranges::find(entry.channels, semantic_id, &LayerEntry::ChannelModulation::semantic_id);
    return found == entry.channels.end() ? nullptr : &*found;
}

class ChannelCompositor {
public:
    ChannelCompositor(const TextureSet& texture_set, const CompositeInputs& inputs,
                      const ChannelDescriptor& descriptor, std::size_t pixel_count)
        : stack_(texture_set.layer_stack()),
          entries_(stack_.entries()),
          inputs_(inputs),
          descriptor_(descriptor),
          semantic_id_(descriptor.semantic_id),
          pixel_count_(pixel_count),
          unity_(pixel_count, 1.0) {
        index_entries();
    }

    [[nodiscard]] LayerCompositeChannel run() {
        CompositeRaster output(pixel_count_, default_sample());
        for (const std::size_t index : roots_) {
            apply_entry(index, output, unity_);
        }
        LayerCompositeChannel result{.semantic_id = std::string(semantic_id_),
                                     .component_count = descriptor_.component_count,
                                     .pixels = {}};
        result.pixels.reserve(pixel_count_);
        for (const CompositeSample& sample : output) {
            result.pixels.push_back(sample.value);
        }
        return result;
    }

private:
    void index_entries() {
        for (std::size_t index = 0; index < entries_.size(); ++index) {
            const LayerEntry& entry = entries_[index];
            indices_.emplace(entry.identifier, index);
            if (entry.kind == LayerEntryKind::mask) {
                masks_[entry.target_identifier].push_back(index);
            } else if (entry.kind == LayerEntryKind::filter) {
                filters_[entry.target_identifier].push_back(index);
            } else if (entry.parent_identifier.empty()) {
                roots_.push_back(index);
            } else {
                children_[entry.parent_identifier].push_back(index);
            }
        }
    }

    [[nodiscard]] CompositeSample default_sample() const {
        std::array<float, 4> value{};
        std::ranges::transform(descriptor_.default_value, value.begin(),
                               [](double component) { return static_cast<float>(component); });
        return {.value = colour(value), .coverage = 1.0F};
    }

    [[nodiscard]] CompositeRaster input_raster(std::string_view entry_identifier) const {
        const LayerCompositeRaster& input = inputs_.content(entry_identifier, semantic_id_);
        CompositeRaster result;
        result.reserve(pixel_count_);
        for (std::size_t pixel = 0; pixel < pixel_count_; ++pixel) {
            result.push_back({.value = input.pixels[pixel],
                              .coverage = input.coverage.empty() ? 1.0F : input.coverage[pixel]});
        }
        return result;
    }

    [[nodiscard]] CompositeRaster group_content(std::size_t index) {
        CompositeRaster result(pixel_count_);
        const auto found = children_.find(entries_[index].identifier);
        if (found == children_.end()) {
            return result;
        }
        for (const std::size_t child : found->second) {
            apply_entry(child, result, unity_);
        }
        return result;
    }

    [[nodiscard]] CompositeRaster base_content(std::size_t index) {
        if (const auto cached = content_cache_.find(index); cached != content_cache_.end()) {
            return cached->second;
        }
        if (!active_content_.insert(index).second) {
            fail(LayerCompositeErrorCode::invalid_request,
                 "layer composite content dependency is recursive at entry '" +
                     entries_[index].identifier + "'");
        }
        const LayerEntry& entry = entries_[index];
        CompositeRaster result;
        if (entry.kind == LayerEntryKind::group) {
            result = group_content(index);
        } else if (entry.kind == LayerEntryKind::instance) {
            result =
                base_content(indices_.at(stack_.resolved_content(entry.identifier).identifier));
        } else {
            result = input_raster(entry.identifier);
        }
        active_content_.erase(index);
        content_cache_.emplace(index, result);
        return result;
    }

    [[nodiscard]] std::vector<double> factors(const LayerEntry& entry,
                                              std::span<const double> inherited,
                                              double local_opacity) const {
        std::vector<double> result(inherited.begin(), inherited.end());
        for (double& factor : result) {
            factor *= local_opacity;
        }
        const auto masks = masks_.find(entry.identifier);
        if (masks == masks_.end()) {
            return result;
        }
        for (const std::size_t mask_index : masks->second) {
            const LayerEntry& mask_entry = entries_[mask_index];
            if (!mask_entry.enabled) {
                continue;
            }
            const LayerCompositeMaskRaster& mask = inputs_.mask(mask_entry.identifier);
            for (std::size_t pixel = 0; pixel < pixel_count_; ++pixel) {
                result[pixel] *= mask.values[pixel];
            }
        }
        return result;
    }

    void blend(CompositeRaster& base, const CompositeRaster& layer, std::span<const double> factor,
               std::string_view mode) const {
        for (std::size_t pixel = 0; pixel < pixel_count_; ++pixel) {
            base[pixel] =
                composite_sample(descriptor_, mode, base[pixel], layer[pixel], factor[pixel]);
        }
    }

    void apply_filters(const LayerEntry& target, CompositeRaster& content) {
        const auto found = filters_.find(target.identifier);
        if (found == filters_.end()) {
            return;
        }
        for (const std::size_t index : found->second) {
            const LayerEntry& filter = entries_[index];
            const LayerEntry::ChannelModulation* channel = channel_modulation(filter, semantic_id_);
            if (!filter.enabled || channel == nullptr || !channel->enabled) {
                continue;
            }
            const CompositeRaster filtered = input_raster(filter.identifier);
            const std::vector<double> factor =
                factors(filter, unity_, filter.opacity * channel->opacity);
            blend(content, filtered, factor, filter.blend_mode);
        }
    }

    [[nodiscard]] CompositeRaster renderable_content(std::size_t index) {
        CompositeRaster result = base_content(index);
        apply_filters(entries_[index], result);
        return result;
    }

    void apply_group(std::size_t index, CompositeRaster& base, std::span<const double> inherited) {
        const LayerEntry& group = entries_[index];
        if (!group.enabled) {
            return;
        }
        const std::vector<double> factor = factors(group, inherited, group.opacity);
        if (group.blend_mode == "pass_through" && !filters_.contains(group.identifier)) {
            const auto found = children_.find(group.identifier);
            if (found != children_.end()) {
                for (const std::size_t child : found->second) {
                    apply_entry(child, base, factor);
                }
            }
            return;
        }
        const CompositeRaster content = renderable_content(index);
        const std::string_view mode =
            group.blend_mode == "pass_through" ? std::string_view{"normal"} : group.blend_mode;
        blend(base, content, factor, mode);
    }

    void apply_entry(std::size_t index, CompositeRaster& base, std::span<const double> inherited) {
        const LayerEntry& entry = entries_[index];
        if (entry.kind == LayerEntryKind::group) {
            apply_group(index, base, inherited);
            return;
        }
        const LayerEntry::ChannelModulation* channel = channel_modulation(entry, semantic_id_);
        if (!entry.enabled || channel == nullptr || !channel->enabled) {
            return;
        }
        const CompositeRaster content = renderable_content(index);
        const std::vector<double> factor =
            factors(entry, inherited, entry.opacity * channel->opacity);
        blend(base, content, factor, entry.blend_mode);
    }

    const LayerStack& stack_;
    std::span<const LayerEntry> entries_;
    const CompositeInputs& inputs_;
    const ChannelDescriptor& descriptor_;
    std::string_view semantic_id_;
    std::size_t pixel_count_;
    std::vector<double> unity_;
    EntryIndices indices_;
    std::vector<std::size_t> roots_;
    std::map<std::string_view, std::vector<std::size_t>, std::less<>> children_;
    std::map<std::string_view, std::vector<std::size_t>, std::less<>> masks_;
    std::map<std::string_view, std::vector<std::size_t>, std::less<>> filters_;
    std::map<std::size_t, CompositeRaster> content_cache_;
    std::set<std::size_t> active_content_;
};

}  // namespace

LayerCompositeError::LayerCompositeError(LayerCompositeErrorCode code, std::string message)
    : std::invalid_argument(std::move(message)), code_(code) {}

const LayerCompositeChannel& LayerCompositeResult::channel(std::string_view semantic_id) const {
    const auto found =
        std::ranges::find(channels, semantic_id, &LayerCompositeChannel::semantic_id);
    if (found == channels.end()) {
        throw std::out_of_range("layer composite channel is not present: " +
                                std::string(semantic_id));
    }
    return *found;
}

LayerCompositeResult composite_texture_set_cpu(const TextureSet& texture_set,
                                               const LayerCompositeRequest& request) {
    const TextureSetDescriptor set_descriptor = texture_set.descriptor();
    if (request.width != set_descriptor.width || request.height != set_descriptor.height) {
        fail(LayerCompositeErrorCode::invalid_request,
             "layer composite dimensions must match the texture set");
    }
    const std::size_t pixel_count = checked_pixel_count(request.width, request.height);
    const CompositeInputs inputs(texture_set, request, pixel_count);
    LayerCompositeResult result{.width = request.width, .height = request.height, .channels = {}};
    for (const std::string& semantic_id : texture_set.channels().semantic_ids()) {
        if (!texture_set.channels().is_enabled(semantic_id)) {
            continue;
        }
        const ChannelDescriptor& descriptor = texture_set.channels().descriptor(semantic_id);
        if (!descriptor.evaluable) {
            fail(LayerCompositeErrorCode::unevaluable_channel,
                 "layer composite cannot evaluate channel '" + semantic_id + "'");
        }
        if (!normalized_defaults(descriptor)) {
            fail(LayerCompositeErrorCode::invalid_request,
                 "layer composite channel default is not finite and normalized: '" + semantic_id +
                     "'");
        }
        result.channels.push_back(
            ChannelCompositor(texture_set, inputs, descriptor, pixel_count).run());
    }
    return result;
}

}  // namespace ctex::doc
