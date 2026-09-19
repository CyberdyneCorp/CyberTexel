#ifndef CTEX_EMIT_KONG_CONTEXT_HPP
#define CTEX_EMIT_KONG_CONTEXT_HPP

#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>

namespace ctex::emit {

struct KongWgslProgram {
    std::string vertex_source;
    std::string fragment_source;
    friend bool operator==(const KongWgslProgram&, const KongWgslProgram&) = default;
};

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

    [[nodiscard]] KongWgslProgram compile_wgsl(std::string_view source);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace ctex::emit

#endif
