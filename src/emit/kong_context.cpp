#include <kong_context.h>

#include <ctex/emit/kong_context.hpp>
#include <mutex>
#include <new>
#include <utility>

namespace ctex::emit {

class KongContext::Impl {
public:
    Impl() : context_(kong_context_create()) {
        if (context_ == nullptr) {
            throw std::bad_alloc();
        }
    }

    ~Impl() { kong_context_destroy(context_); }

    [[nodiscard]] KongWgslProgram compile(std::string_view source) {
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
        char* vertex = nullptr;
        char* fragment = nullptr;
        if (!kong_context_compile_wgsl(context_, terminated.c_str(), &vertex, &fragment)) {
            const std::string message = kong_context_last_error(context_);
            kong_context_free_string(vertex);
            kong_context_free_string(fragment);
            throw KongCompilationError(message.empty() ? "Kong compilation failed" : message);
        }
        KongWgslProgram result{vertex, fragment};
        kong_context_free_string(vertex);
        kong_context_free_string(fragment);
        return result;
    }

private:
    std::mutex mutex_;
    kong_context* context_{};
    bool used_{};
};

KongContext::KongContext() : impl_(std::make_unique<Impl>()) {}
KongContext::~KongContext() = default;
KongContext::KongContext(KongContext&&) noexcept = default;
KongContext& KongContext::operator=(KongContext&&) noexcept = default;

KongWgslProgram KongContext::compile_wgsl(std::string_view source) {
    if (impl_ == nullptr) {
        throw std::logic_error("cannot use a moved-from Kong context");
    }
    return impl_->compile(source);
}

}  // namespace ctex::emit
