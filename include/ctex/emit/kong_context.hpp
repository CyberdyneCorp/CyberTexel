#ifndef CTEX_EMIT_KONG_CONTEXT_HPP
#define CTEX_EMIT_KONG_CONTEXT_HPP

#include <ctex/graph/host_nodes.hpp>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace ctex::emit {

struct KongWgslProgram {
    std::string vertex_source;
    std::string fragment_source;
    friend bool operator==(const KongWgslProgram&, const KongWgslProgram&) = default;
};

using ShaderTarget = graph::EmissionTarget;

struct SplitTextShaderProgram {
    std::string vertex_source;
    std::string fragment_source;
    friend bool operator==(const SplitTextShaderProgram&, const SplitTextShaderProgram&) = default;
};

struct UnifiedTextShaderProgram {
    std::string source;
    friend bool operator==(const UnifiedTextShaderProgram&,
                           const UnifiedTextShaderProgram&) = default;
};

struct SpirvShaderProgram {
    std::vector<std::uint32_t> vertex_module;
    std::vector<std::uint32_t> fragment_module;
    friend bool operator==(const SpirvShaderProgram&, const SpirvShaderProgram&) = default;
};

using KongShaderPayload =
    std::variant<SplitTextShaderProgram, UnifiedTextShaderProgram, SpirvShaderProgram>;

struct KongShaderProgram {
    ShaderTarget target{};
    KongShaderPayload payload;
    friend bool operator==(const KongShaderProgram&, const KongShaderProgram&) = default;
};

[[nodiscard]] std::string_view shader_target_name(ShaderTarget target) noexcept;
[[nodiscard]] std::span<const ShaderTarget> supported_shader_targets() noexcept;

class KongCompilationError final : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class KongContext {
public:
    KongContext();
    ~KongContext();
    KongContext(KongContext&&) noexcept;
    KongContext& operator=(KongContext&&) noexcept;
    KongContext(const KongContext&) = delete;
    KongContext& operator=(const KongContext&) = delete;

    [[nodiscard]] KongShaderProgram compile(std::string_view source, ShaderTarget target);
    [[nodiscard]] KongWgslProgram compile_wgsl(std::string_view source);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace ctex::emit

#endif
