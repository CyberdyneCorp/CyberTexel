#include <ctex/emit/kong_context.hpp>
#include <future>
#include <iostream>
#include <string>
#include <string_view>

namespace {

constexpr std::string_view red_shader = R"(
struct vert_in {
    pos: float3;
}

struct vert_out {
    pos: float4;
}

fun red_vert(input: vert_in): vert_out {
    var output: vert_out;
    output.pos = float4(input.pos, 1.0);
    return output;
}

fun red_frag(input: vert_out): float4 {
    return float4(0.25, 0.0, 0.0, 1.0);
}

#[pipe]
struct red_pipe {
    vertex = red_vert;
    fragment = red_frag;
}
)";

constexpr std::string_view blue_shader = R"(
struct vert_in {
    pos: float3;
}

struct vert_out {
    pos: float4;
}

fun blue_vert(input: vert_in): vert_out {
    var output: vert_out;
    output.pos = float4(input.pos, 1.0);
    return output;
}

fun blue_frag(input: vert_out): float4 {
    return float4(0.0, 0.0, 0.75, 1.0);
}

#[pipe]
struct blue_pipe {
    vertex = blue_vert;
    fragment = blue_frag;
}
)";

bool expect(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

bool isolated_contexts_compile_concurrently() {
    auto red = std::async(std::launch::async, [] {
        ctex::emit::KongContext context;
        return context.compile_wgsl(red_shader);
    });
    auto blue = std::async(std::launch::async, [] {
        ctex::emit::KongContext context;
        return context.compile_wgsl(blue_shader);
    });
    const auto red_program = red.get();
    const auto blue_program = blue.get();
    const bool red_isolated =
        red_program.vertex_source.find("@vertex fn main") != std::string::npos &&
        red_program.fragment_source.find("0.250000") != std::string::npos &&
        red_program.fragment_source.find("0.750000") == std::string::npos;
    const bool blue_isolated =
        blue_program.vertex_source.find("@vertex fn main") != std::string::npos &&
        blue_program.fragment_source.find("0.750000") != std::string::npos &&
        blue_program.fragment_source.find("0.250000") == std::string::npos;
    if (!red_isolated) {
        std::cerr << "red vertex:\n"
                  << red_program.vertex_source << "\nred fragment:\n"
                  << red_program.fragment_source << '\n';
    }
    if (!blue_isolated) {
        std::cerr << "blue vertex:\n"
                  << blue_program.vertex_source << "\nblue fragment:\n"
                  << blue_program.fragment_source << '\n';
    }
    return expect(red_isolated, "red context was missing its shader or observed blue state") &&
           expect(blue_isolated, "blue context was missing its shader or observed red state");
}

bool one_wrapper_context_resets_between_compilations() {
    ctex::emit::KongContext context;
    const auto first = context.compile_wgsl(red_shader);
    const auto second = context.compile_wgsl(blue_shader);
    const auto repeated = context.compile_wgsl(red_shader);
    return expect(first == repeated, "reused wrapper context changed deterministic output") &&
           expect(second.fragment_source.find("0.250000") == std::string::npos &&
                      second.fragment_source.find("0.750000") != std::string::npos,
                  "reused wrapper context retained the preceding compiler state");
}

}  // namespace

int main() {
    return isolated_contexts_compile_concurrently() &&
                   one_wrapper_context_resets_between_compilations()
               ? 0
               : 1;
}
