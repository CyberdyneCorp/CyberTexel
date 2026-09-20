#ifndef CTEX_DOC_LAYER_STACK_HPP
#define CTEX_DOC_LAYER_STACK_HPP

#include <cstddef>
#include <cstdint>
#include <ctex/graph/document.hpp>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace ctex::doc {

enum class LayerEntryKind : std::uint8_t {
    paint_layer,
    fill_layer,
    group,
    mask,
    filter,
    instance,
    editable_decal,
    editable_text,
    surface_path,
};

struct LayerEntry {
    std::string identifier;
    std::string display_name;
    LayerEntryKind kind{LayerEntryKind::paint_layer};
    std::string parent_identifier;
    std::string target_identifier;
    std::string source_identifier;
    bool enabled{true};
    double opacity{1.0};
    std::string blend_mode{"normal"};
    struct ChannelModulation {
        std::string semantic_id;
        bool enabled{true};
        double opacity{1.0};
        friend bool operator==(const ChannelModulation&, const ChannelModulation&) = default;
    };
    std::vector<ChannelModulation> channels;
    std::optional<graph::GraphDocument> graph;
    std::uint64_t content_revision{1};
    friend bool operator==(const LayerEntry&, const LayerEntry&) = default;
};

enum class LayerStackRule : std::uint8_t {
    identity,
    evaluation_order,
    group_parent,
    maximum_group_depth,
    single_attachment,
    attachment_target,
    instance_source,
    instance_cycle,
    direct_instance_paint,
    live_instances,
    blend_mode,
    entry_content,
};

class LayerStackError final : public std::invalid_argument {
public:
    LayerStackError(LayerStackRule rule, std::string message,
                    std::vector<std::string> related_identifiers = {});
    [[nodiscard]] LayerStackRule rule() const noexcept { return rule_; }
    [[nodiscard]] std::span<const std::string> related_identifiers() const noexcept {
        return related_identifiers_;
    }

private:
    LayerStackRule rule_;
    std::vector<std::string> related_identifiers_;
};

enum class ReferencedSourceDeletionPolicy : std::uint8_t { refuse, make_instances_independent };

class LayerStack {
public:
    static constexpr std::size_t maximum_group_depth = 32;

    [[nodiscard]] std::span<const LayerEntry> entries() const noexcept { return entries_; }
    [[nodiscard]] std::size_t size() const noexcept { return entries_.size(); }
    [[nodiscard]] bool empty() const noexcept { return entries_.empty(); }
    [[nodiscard]] bool contains(std::string_view identifier) const noexcept;
    [[nodiscard]] const LayerEntry& entry(std::string_view identifier) const;
    [[nodiscard]] const LayerEntry& resolved_content(std::string_view identifier) const;
    [[nodiscard]] const LayerEntry& paint_target(std::string_view identifier) const;
    [[nodiscard]] graph::ColourValue evaluate_blend(std::string_view identifier,
                                                    graph::ColourValue base,
                                                    graph::ColourValue layer, double factor) const;

    void append(LayerEntry entry);
    void append(std::span<const LayerEntry> entries);
    void replace(std::string_view identifier, LayerEntry replacement);
    void set_layout(std::string_view identifier, std::string parent_identifier,
                    std::string target_identifier);
    void set_fill_graph(std::string_view identifier, graph::GraphDocument graph);
    void record_paint(std::string_view identifier);
    void set_opacity(std::string_view identifier, double opacity);
    void set_blend_mode(std::string_view identifier, std::string blend_mode);
    void set_channel_modulation(std::string_view identifier, LayerEntry::ChannelModulation channel);
    void remove(std::span<const std::string> identifiers,
                ReferencedSourceDeletionPolicy policy = ReferencedSourceDeletionPolicy::refuse);

    friend bool operator==(const LayerStack&, const LayerStack&) = default;

private:
    static void validate(std::span<const LayerEntry> entries);
    std::vector<LayerEntry> entries_;
};

}  // namespace ctex::doc

#endif
