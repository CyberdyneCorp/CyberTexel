#include <kong_context.h>

#include <array>
#include <cstring>
#include <ctex/emit/kong_context.hpp>
#include <mutex>
#include <new>
#include <type_traits>
#include <utility>

namespace ctex::emit {

static_assert(static_cast<int>(ShaderTarget::wgsl) == KONG_TARGET_WGSL);
static_assert(static_cast<int>(ShaderTarget::msl) == KONG_TARGET_MSL);
static_assert(static_cast<int>(ShaderTarget::spirv) == KONG_TARGET_SPIRV);
static_assert(static_cast<int>(ShaderTarget::hlsl) == KONG_TARGET_HLSL);

class KongContext::Impl {
public:
    Impl() : context_(kong_context_create()) {
        if (context_ == nullptr) {
            throw std::bad_alloc();
        }
    }

    ~Impl() { kong_context_destroy(context_); }

    [[nodiscard]] KongShaderProgram compile(std::string_view source, ShaderTarget target) {
        const std::scoped_lock lock(mutex_);
        if (used_) {
            kong_context* replacement = kong_context_create();
            if (replacement == nullptr) {
                throw std::bad_alloc();
            }
            kong_context_destroy(context_);
            context_ = replacement;
        }
        used_ = true;

        const std::string terminated(source);
        kong_compilation compilation{};
        const auto raw_target =
            static_cast<kong_target>(static_cast<std::underlying_type_t<ShaderTarget>>(target));
        if (!kong_context_compile(context_, terminated.c_str(), raw_target, &compilation)) {
            const std::string message = kong_context_last_error(context_);
            throw KongCompilationError(message.empty() ? "Kong compilation failed" : message);
        }
        try {
            KongShaderProgram result{target, make_payload(target, compilation)};
            kong_compilation_destroy(&compilation);
            return result;
        } catch (...) {
            kong_compilation_destroy(&compilation);
            throw;
        }
    }

private:
    [[nodiscard]] static std::vector<std::uint32_t> copy_words(const std::uint8_t* bytes,
                                                               std::size_t size) {
        if (bytes == nullptr || size == 0 || size % sizeof(std::uint32_t) != 0) {
            throw KongCompilationError("Kong SPIR-V backend returned malformed binary output");
        }
        std::vector<std::uint32_t> words(size / sizeof(std::uint32_t));
        std::memcpy(words.data(), bytes, size);
        return words;
    }

    [[nodiscard]] static KongShaderPayload make_payload(ShaderTarget target,
                                                        const kong_compilation& compilation) {
        switch (target) {
            case ShaderTarget::wgsl:
            case ShaderTarget::hlsl:
                return SplitTextShaderProgram{compilation.vertex_source,
                                              compilation.fragment_source};
            case ShaderTarget::msl:
                return UnifiedTextShaderProgram{compilation.module_source};
            case ShaderTarget::spirv:
                return SpirvShaderProgram{
                    copy_words(compilation.vertex_binary, compilation.vertex_binary_size),
                    copy_words(compilation.fragment_binary, compilation.fragment_binary_size)};
        }
        throw KongCompilationError("unsupported shader target payload");
    }

    std::mutex mutex_;
    kong_context* context_{};
    bool used_{};
};

KongContext::KongContext() : impl_(std::make_unique<Impl>()) {}
KongContext::~KongContext() = default;
KongContext::KongContext(KongContext&&) noexcept = default;
KongContext& KongContext::operator=(KongContext&&) noexcept = default;

std::string_view shader_target_name(ShaderTarget target) noexcept {
    switch (target) {
        case ShaderTarget::wgsl:
            return "WGSL";
        case ShaderTarget::msl:
            return "MSL";
        case ShaderTarget::spirv:
            return "SPIR-V";
        case ShaderTarget::hlsl:
            return "HLSL";
    }
    return "unknown";
}

std::span<const ShaderTarget> supported_shader_targets() noexcept {
    static constexpr std::array targets{ShaderTarget::wgsl, ShaderTarget::msl, ShaderTarget::spirv,
                                        ShaderTarget::hlsl};
    return targets;
}

KongShaderProgram KongContext::compile(std::string_view source, ShaderTarget target) {
    if (impl_ == nullptr) {
        throw std::logic_error("cannot use a moved-from Kong context");
    }
    return impl_->compile(source, target);
}

KongWgslProgram KongContext::compile_wgsl(std::string_view source) {
    KongShaderProgram program = compile(source, ShaderTarget::wgsl);
    auto& text = std::get<SplitTextShaderProgram>(program.payload);
    return {std::move(text.vertex_source), std::move(text.fragment_source)};
}

}  // namespace ctex::emit
