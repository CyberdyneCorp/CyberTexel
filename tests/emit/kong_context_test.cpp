#include <algorithm>
#include <array>
#include <cstdlib>
#include <ctex/emit/kong_context.hpp>
#include <filesystem>
#include <fstream>
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

bool every_target_has_the_declared_artifact_kind() {
    ctex::emit::KongContext context;
    const auto wgsl = context.compile(red_shader, ctex::emit::ShaderTarget::wgsl);
    const auto msl = context.compile(red_shader, ctex::emit::ShaderTarget::msl);
    const auto spirv = context.compile(red_shader, ctex::emit::ShaderTarget::spirv);
    const auto hlsl = context.compile(red_shader, ctex::emit::ShaderTarget::hlsl);

    const auto& wgsl_text = std::get<ctex::emit::SplitTextShaderProgram>(wgsl.payload);
    const auto& msl_text = std::get<ctex::emit::UnifiedTextShaderProgram>(msl.payload);
    const auto& spirv_binary = std::get<ctex::emit::SpirvShaderProgram>(spirv.payload);
    const auto& hlsl_text = std::get<ctex::emit::SplitTextShaderProgram>(hlsl.payload);
    constexpr std::uint32_t spirv_magic = 0x07230203;
    return expect(wgsl_text.vertex_source.find("@vertex fn main") != std::string::npos &&
                      wgsl_text.fragment_source.find("0.250000") != std::string::npos,
                  "WGSL target did not return split textual stages") &&
           expect(msl_text.source.find("#include <metal_stdlib>") != std::string::npos &&
                      msl_text.source.find("0.250000") != std::string::npos,
                  "MSL target did not return one textual module") &&
           expect(!spirv_binary.vertex_module.empty() && !spirv_binary.fragment_module.empty() &&
                      spirv_binary.vertex_module.front() == spirv_magic &&
                      spirv_binary.fragment_module.front() == spirv_magic,
                  "SPIR-V target did not return two binary modules") &&
           expect(hlsl_text.vertex_source.find("SV_POSITION") != std::string::npos &&
                      hlsl_text.fragment_source.find("0.250000") != std::string::npos,
                  "HLSL target did not return split textual stages");
}

bool target_inventory_and_refusal_are_explicit() {
    constexpr std::array expected{ctex::emit::ShaderTarget::wgsl, ctex::emit::ShaderTarget::msl,
                                  ctex::emit::ShaderTarget::spirv, ctex::emit::ShaderTarget::hlsl};
    const auto supported = ctex::emit::supported_shader_targets();
    if (!expect(std::ranges::equal(supported, expected) &&
                    ctex::emit::shader_target_name(supported[0]) == "WGSL" &&
                    ctex::emit::shader_target_name(supported[1]) == "MSL" &&
                    ctex::emit::shader_target_name(supported[2]) == "SPIR-V" &&
                    ctex::emit::shader_target_name(supported[3]) == "HLSL",
                "supported shader target inventory or names were incomplete or unstable")) {
        return false;
    }
    try {
        ctex::emit::KongContext context;
        static_cast<void>(context.compile(red_shader, static_cast<ctex::emit::ShaderTarget>(99)));
    } catch (const ctex::emit::KongCompilationError& error) {
        const std::string_view message(error.what());
        return expect(message.find("unknown(99)") != std::string_view::npos &&
                          message.find("WGSL, MSL, SPIR-V, HLSL") != std::string_view::npos,
                      "unsupported-target error did not name the request and available targets");
    }
    return expect(false, "an unknown shader target was accepted");
}

bool every_target_is_deterministic() {
    for (const ctex::emit::ShaderTarget target : ctex::emit::supported_shader_targets()) {
        ctex::emit::KongContext first_context;
        ctex::emit::KongContext second_context;
        const auto first = first_context.compile(red_shader, target);
        const auto second = second_context.compile(red_shader, target);
        if (!expect(first == second, "a shader target changed output across identical contexts")) {
            return false;
        }
    }
    return true;
}

bool different_targets_compile_concurrently() {
    const auto compile = [](ctex::emit::ShaderTarget target) {
        return std::async(std::launch::async, [target] {
            ctex::emit::KongContext context;
            return context.compile(red_shader, target);
        });
    };
    auto wgsl = compile(ctex::emit::ShaderTarget::wgsl);
    auto msl = compile(ctex::emit::ShaderTarget::msl);
    auto spirv = compile(ctex::emit::ShaderTarget::spirv);
    auto hlsl = compile(ctex::emit::ShaderTarget::hlsl);
    return expect(wgsl.get().target == ctex::emit::ShaderTarget::wgsl &&
                      msl.get().target == ctex::emit::ShaderTarget::msl &&
                      spirv.get().target == ctex::emit::ShaderTarget::spirv &&
                      hlsl.get().target == ctex::emit::ShaderTarget::hlsl,
                  "concurrent target compilation mixed context state");
}

bool write_spirv_modules(std::string_view vertex_path, std::string_view fragment_path) {
    ctex::emit::KongContext context;
    const auto program = context.compile(red_shader, ctex::emit::ShaderTarget::spirv);
    const auto& binary = std::get<ctex::emit::SpirvShaderProgram>(program.payload);
    const auto write = [](std::string_view path, std::span<const std::uint32_t> words) {
        std::ofstream output(std::string(path), std::ios::binary);
        output.write(reinterpret_cast<const char*>(words.data()),
                     static_cast<std::streamsize>(words.size_bytes()));
        return output.good();
    };
    return expect(
        write(vertex_path, binary.vertex_module) && write(fragment_path, binary.fragment_module),
        "could not write SPIR-V validation fixtures");
}

bool write_target_artifacts() {
    const char* directory = std::getenv("CTEX_DETERMINISM_OUTPUT_DIR");
    if (directory == nullptr) {
        return expect(false, "CTEX_DETERMINISM_OUTPUT_DIR is required for artifact output");
    }
    const std::filesystem::path root(directory);
    const auto write_text = [&](std::string_view name, std::string_view source) {
        std::ofstream output(root / name, std::ios::binary);
        output.write(source.data(), static_cast<std::streamsize>(source.size()));
        return output.good();
    };
    const auto write_words = [&](std::string_view name, std::span<const std::uint32_t> words) {
        std::ofstream output(root / name, std::ios::binary);
        output.write(reinterpret_cast<const char*>(words.data()),
                     static_cast<std::streamsize>(words.size_bytes()));
        return output.good();
    };

    ctex::emit::KongContext context;
    const auto wgsl = std::get<ctex::emit::SplitTextShaderProgram>(
        context.compile(red_shader, ctex::emit::ShaderTarget::wgsl).payload);
    const auto msl = std::get<ctex::emit::UnifiedTextShaderProgram>(
        context.compile(red_shader, ctex::emit::ShaderTarget::msl).payload);
    const auto spirv = std::get<ctex::emit::SpirvShaderProgram>(
        context.compile(red_shader, ctex::emit::ShaderTarget::spirv).payload);
    const auto hlsl = std::get<ctex::emit::SplitTextShaderProgram>(
        context.compile(red_shader, ctex::emit::ShaderTarget::hlsl).payload);
    return expect(write_text("kong.wgsl.vert", wgsl.vertex_source) &&
                      write_text("kong.wgsl.frag", wgsl.fragment_source) &&
                      write_text("kong.msl", msl.source) &&
                      write_words("kong.spirv.vert", spirv.vertex_module) &&
                      write_words("kong.spirv.frag", spirv.fragment_module) &&
                      write_text("kong.hlsl.vert", hlsl.vertex_source) &&
                      write_text("kong.hlsl.frag", hlsl.fragment_source),
                  "could not write deterministic target artifacts");
}

}  // namespace

int main(int argc, char** argv) {
    if (argc == 4 && std::string_view(argv[1]) == "--write-spirv") {
        return write_spirv_modules(argv[2], argv[3]) ? 0 : 1;
    }
    if (argc == 2 && std::string_view(argv[1]) == "--write-target-artifacts") {
        return write_target_artifacts() ? 0 : 1;
    }
    return isolated_contexts_compile_concurrently() &&
                   one_wrapper_context_resets_between_compilations() &&
                   every_target_has_the_declared_artifact_kind() &&
                   target_inventory_and_refusal_are_explicit() && every_target_is_deterministic() &&
                   different_targets_compile_concurrently()
               ? 0
               : 1;
}
