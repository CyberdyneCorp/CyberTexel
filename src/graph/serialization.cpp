#include <algorithm>
#include <bit>
#include <charconv>
#include <ctex/graph/document.hpp>
#include <limits>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace ctex::graph {
namespace {

constexpr char hex_digits[] = "0123456789abcdef";

struct EncodedValue {
    std::string_view tag;
    std::string payload;
};

std::string encode_bytes(std::string_view value) {
    std::string result;
    result.reserve(value.size() * 2);
    for (unsigned char byte : value) {
        result.push_back(hex_digits[byte >> 4]);
        result.push_back(hex_digits[byte & 0x0F]);
    }
    return result;
}

template <typename Integer>
std::string fixed_hex(Integer value) {
    static_assert(std::is_unsigned_v<Integer>);
    std::string result(sizeof(Integer) * 2, '0');
    for (std::size_t index = result.size(); index != 0; --index) {
        result[index - 1] = hex_digits[value & 0x0F];
        value >>= 4;
    }
    return result;
}

std::string float_hex(float value) { return fixed_hex(std::bit_cast<std::uint32_t>(value)); }

std::string double_hex(double value) { return fixed_hex(std::bit_cast<std::uint64_t>(value)); }

EncodedValue encode_value(const SocketValue& value) {
    if (std::holds_alternative<std::monostate>(value)) {
        return {"none", {}};
    }
    if (const auto* boolean = std::get_if<bool>(&value)) {
        return {"bool", *boolean ? "1" : "0"};
    }
    if (const auto* scalar = std::get_if<double>(&value)) {
        return {"scalar", double_hex(*scalar)};
    }
    if (const auto* vector = std::get_if<VectorValue>(&value)) {
        return {"vector", float_hex(vector->x) + float_hex(vector->y) + float_hex(vector->z)};
    }
    if (const auto* colour = std::get_if<ColourValue>(&value)) {
        return {"colour", float_hex(colour->r) + float_hex(colour->g) + float_hex(colour->b) +
                              float_hex(colour->a)};
    }
    if (const auto* text = std::get_if<std::string>(&value)) {
        return {"text", encode_bytes(*text)};
    }
    return {"image", encode_bytes(std::get<ImageValue>(value).resource_id)};
}

void append_field(std::string& result, std::string_view field) {
    result.push_back('\t');
    result.append(field);
}

void append_socket(std::string& result, std::string_view record, NodeId node_id,
                   const NodeSocket& socket) {
    result.append(record);
    append_field(result, std::to_string(node_id));
    append_field(result, std::to_string(static_cast<unsigned>(socket.type)));
    append_field(result, encode_bytes(socket.identifier));
    append_field(result, encode_bytes(socket.display_name));
    const EncodedValue encoded = encode_value(socket.value);
    append_field(result, encoded.tag);
    append_field(result, encoded.payload);
    result.push_back('\n');
}

std::vector<std::string_view> split(std::string_view value, char delimiter) {
    std::vector<std::string_view> result;
    std::size_t begin = 0;
    while (true) {
        const std::size_t end = value.find(delimiter, begin);
        if (end == std::string_view::npos) {
            result.push_back(value.substr(begin));
            return result;
        }
        result.push_back(value.substr(begin, end - begin));
        begin = end + 1;
    }
}

std::vector<std::string_view> lines(std::string_view serialized) {
    std::vector<std::string_view> result = split(serialized, '\n');
    if (!result.empty() && result.back().empty()) {
        result.pop_back();
    }
    for (std::string_view line : result) {
        if (line.empty()) {
            throw std::invalid_argument("serialized graph contains an empty record");
        }
    }
    return result;
}

std::uint8_t hex_nibble(char value) {
    if (value >= '0' && value <= '9') {
        return static_cast<std::uint8_t>(value - '0');
    }
    if (value >= 'a' && value <= 'f') {
        return static_cast<std::uint8_t>(value - 'a' + 10);
    }
    throw std::invalid_argument("serialized graph contains invalid hexadecimal data");
}

std::string decode_bytes(std::string_view encoded) {
    if (encoded.size() % 2 != 0) {
        throw std::invalid_argument("serialized graph contains an odd hexadecimal string");
    }
    std::string result;
    result.reserve(encoded.size() / 2);
    for (std::size_t index = 0; index < encoded.size(); index += 2) {
        result.push_back(
            static_cast<char>((hex_nibble(encoded[index]) << 4) | hex_nibble(encoded[index + 1])));
    }
    return result;
}

template <typename Integer>
Integer parse_decimal(std::string_view field, std::string_view name) {
    static_assert(std::is_unsigned_v<Integer>);
    std::uint64_t parsed = 0;
    const auto [end, error] = std::from_chars(field.data(), field.data() + field.size(), parsed);
    if (error != std::errc{} || end != field.data() + field.size() ||
        parsed > std::numeric_limits<Integer>::max()) {
        throw std::invalid_argument("serialized graph has an invalid " + std::string(name));
    }
    return static_cast<Integer>(parsed);
}

template <typename Integer>
Integer parse_fixed_hex(std::string_view field) {
    static_assert(std::is_unsigned_v<Integer>);
    if (field.size() != sizeof(Integer) * 2) {
        throw std::invalid_argument("serialized graph has a malformed numeric payload");
    }
    Integer result = 0;
    for (char digit : field) {
        result = static_cast<Integer>((result << 4) | hex_nibble(digit));
    }
    return result;
}

float parse_float(std::string_view field) {
    return std::bit_cast<float>(parse_fixed_hex<std::uint32_t>(field));
}

double parse_double(std::string_view field) {
    return std::bit_cast<double>(parse_fixed_hex<std::uint64_t>(field));
}

SocketValue parse_value(std::string_view tag, std::string_view payload) {
    if (tag == "none" && payload.empty()) {
        return std::monostate{};
    }
    if (tag == "bool" && (payload == "0" || payload == "1")) {
        return payload == "1";
    }
    if (tag == "scalar") {
        return parse_double(payload);
    }
    if (tag == "vector" && payload.size() == 24) {
        return VectorValue{parse_float(payload.substr(0, 8)), parse_float(payload.substr(8, 8)),
                           parse_float(payload.substr(16, 8))};
    }
    if (tag == "colour" && payload.size() == 32) {
        return ColourValue{parse_float(payload.substr(0, 8)), parse_float(payload.substr(8, 8)),
                           parse_float(payload.substr(16, 8)), parse_float(payload.substr(24, 8))};
    }
    if (tag == "text") {
        return decode_bytes(payload);
    }
    if (tag == "image") {
        return ImageValue{decode_bytes(payload)};
    }
    throw std::invalid_argument("serialized graph has an invalid socket value");
}

NodeRole parse_role(std::string_view field) {
    const auto value = parse_decimal<std::uint8_t>(field, "node role");
    if (value > static_cast<std::uint8_t>(NodeRole::output)) {
        throw std::invalid_argument("serialized graph has an unknown node role");
    }
    return static_cast<NodeRole>(value);
}

SocketType parse_socket_type(std::string_view field) {
    const auto value = parse_decimal<std::uint8_t>(field, "socket type");
    if (value > static_cast<std::uint8_t>(SocketType::boolean)) {
        throw std::invalid_argument("serialized graph has an unknown socket type");
    }
    return static_cast<SocketType>(value);
}

GraphNode& find_node(std::vector<GraphNode>& nodes, NodeId id) {
    const auto found = std::find_if(nodes.begin(), nodes.end(),
                                    [&](const GraphNode& node) { return node.id == id; });
    if (found == nodes.end()) {
        throw std::invalid_argument("serialized graph socket references an unknown node");
    }
    return *found;
}

void parse_node_record(std::span<const std::string_view> fields, std::vector<GraphNode>& nodes) {
    if (fields.size() != 8) {
        throw std::invalid_argument("serialized graph has a malformed node record");
    }
    const NodeId id = parse_decimal<NodeId>(fields[1], "node identity");
    if (std::any_of(nodes.begin(), nodes.end(),
                    [&](const GraphNode& node) { return node.id == id; })) {
        throw std::invalid_argument("serialized graph repeats a node identity");
    }
    nodes.push_back({
        .id = id,
        .role = parse_role(fields[2]),
        .type_id = decode_bytes(fields[6]),
        .type_version = parse_decimal<std::uint32_t>(fields[3], "node type version"),
        .display_name = decode_bytes(fields[7]),
        .position = {parse_float(fields[4]), parse_float(fields[5])},
        .inputs = {},
        .outputs = {},
        .properties = {},
    });
}

void parse_socket_record(std::span<const std::string_view> fields, std::vector<GraphNode>& nodes,
                         bool input) {
    if (fields.size() != 7) {
        throw std::invalid_argument("serialized graph has a malformed socket record");
    }
    GraphNode& node = find_node(nodes, parse_decimal<NodeId>(fields[1], "socket node identity"));
    NodeSocket socket{
        .identifier = decode_bytes(fields[3]),
        .display_name = decode_bytes(fields[4]),
        .type = parse_socket_type(fields[2]),
        .value = parse_value(fields[5], fields[6]),
    };
    (input ? node.inputs : node.outputs).push_back(std::move(socket));
}

void parse_property_record(std::span<const std::string_view> fields,
                           std::vector<GraphNode>& nodes) {
    if (fields.size() != 5) {
        throw std::invalid_argument("serialized graph has a malformed property record");
    }
    GraphNode& node = find_node(nodes, parse_decimal<NodeId>(fields[1], "property node identity"));
    node.properties.push_back({decode_bytes(fields[2]), parse_value(fields[3], fields[4])});
}

GraphLink parse_link_record(std::span<const std::string_view> fields) {
    if (fields.size() != 5) {
        throw std::invalid_argument("serialized graph has a malformed link record");
    }
    return {
        .source_node = parse_decimal<NodeId>(fields[1], "link source identity"),
        .source_socket = decode_bytes(fields[2]),
        .target_node = parse_decimal<NodeId>(fields[3], "link target identity"),
        .target_socket = decode_bytes(fields[4]),
    };
}

struct ParseState {
    std::vector<GraphNode> nodes;
    std::vector<GraphLink> links;
    NodeId next_node_id{};
    bool saw_next{};
    bool saw_end{};
};

void parse_record(std::span<const std::string_view> fields, ParseState& state) {
    if (state.saw_end) {
        throw std::invalid_argument("serialized graph has records after END");
    }
    if (fields[0] == "NEXT" && fields.size() == 2 && !state.saw_next) {
        state.next_node_id = parse_decimal<NodeId>(fields[1], "next node identity");
        state.saw_next = true;
        return;
    }
    if (fields[0] == "NODE") {
        parse_node_record(fields, state.nodes);
        return;
    }
    if (fields[0] == "INPUT" || fields[0] == "OUTPUT") {
        parse_socket_record(fields, state.nodes, fields[0] == "INPUT");
        return;
    }
    if (fields[0] == "PROPERTY") {
        parse_property_record(fields, state.nodes);
        return;
    }
    if (fields[0] == "LINK") {
        state.links.push_back(parse_link_record(fields));
        return;
    }
    if (fields[0] == "END" && fields.size() == 1) {
        state.saw_end = true;
        return;
    }
    throw std::invalid_argument("serialized graph contains an unknown or malformed record");
}

}  // namespace

std::string serialize_graph(const GraphDocument& graph) {
    std::string result = "CTEX_GRAPH\t1\nNEXT";
    append_field(result, std::to_string(graph.next_node_id_));
    result.push_back('\n');
    for (const GraphNode& node : graph.nodes_) {
        result.append("NODE");
        append_field(result, std::to_string(node.id));
        append_field(result, std::to_string(static_cast<unsigned>(node.role)));
        append_field(result, std::to_string(node.type_version));
        append_field(result, float_hex(node.position.x));
        append_field(result, float_hex(node.position.y));
        append_field(result, encode_bytes(node.type_id));
        append_field(result, encode_bytes(node.display_name));
        result.push_back('\n');
        for (const NodeSocket& socket : node.inputs) {
            append_socket(result, "INPUT", node.id, socket);
        }
        for (const NodeSocket& socket : node.outputs) {
            append_socket(result, "OUTPUT", node.id, socket);
        }
        for (const NodeProperty& property : node.properties) {
            result.append("PROPERTY");
            append_field(result, std::to_string(node.id));
            append_field(result, encode_bytes(property.key));
            const EncodedValue encoded = encode_value(property.value);
            append_field(result, encoded.tag);
            append_field(result, encoded.payload);
            result.push_back('\n');
        }
    }
    for (const GraphLink& link : graph.links_) {
        result.append("LINK");
        append_field(result, std::to_string(link.source_node));
        append_field(result, encode_bytes(link.source_socket));
        append_field(result, std::to_string(link.target_node));
        append_field(result, encode_bytes(link.target_socket));
        result.push_back('\n');
    }
    result.append("END\n");
    return result;
}

GraphDocument deserialize_graph(std::string_view serialized) {
    const auto records = lines(serialized);
    if (records.empty() || records.front() != "CTEX_GRAPH\t1") {
        throw std::invalid_argument("serialized graph has an unsupported header or version");
    }
    ParseState state;
    for (std::size_t record_index = 1; record_index < records.size(); ++record_index) {
        const auto fields = split(records[record_index], '\t');
        parse_record(fields, state);
    }
    if (!state.saw_next || !state.saw_end) {
        throw std::invalid_argument("serialized graph is missing NEXT or END");
    }
    return GraphDocument(std::move(state.nodes), std::move(state.links), state.next_node_id,
                         GraphDocument::SerializedTag{});
}

}  // namespace ctex::graph
