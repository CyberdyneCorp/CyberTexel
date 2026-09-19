#ifndef CTEX_EXEC_CPU_REFERENCE_HPP
#define CTEX_EXEC_CPU_REFERENCE_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <ctex/exec/execution_control.hpp>
#include <ctex/exec/executor.hpp>
#include <limits>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace ctex::exec {

struct CpuVec2f {
    float x;
    float y;
    friend constexpr bool operator==(CpuVec2f, CpuVec2f) noexcept = default;
};

struct CpuVec3f {
    float x;
    float y;
    float z;
    friend constexpr bool operator==(CpuVec3f, CpuVec3f) noexcept = default;
};

struct CpuMat4f {
    // Column-major: element at row r, column c is values[c * 4 + r].
    std::array<float, 16> values;
};

struct CpuRasterMeshView {
    std::span<const CpuVec3f> positions;
    std::span<const CpuVec2f> uv;
    std::span<const std::uint32_t> triangle_indices;
};

struct CpuRasterCamera {
    CpuMat4f view_projection;
    std::uint32_t width;
    std::uint32_t height;
};

inline constexpr std::uint32_t no_raster_triangle = std::numeric_limits<std::uint32_t>::max();

struct CpuViewportRaster {
    std::uint32_t width{};
    std::uint32_t height{};
    // Depth is OpenGL NDC mapped to [0, 1], with 1 as the uncovered clear value.
    std::vector<float> depth;
    std::vector<CpuVec2f> uv;
    std::vector<std::uint8_t> coverage;
    std::vector<std::uint32_t> triangle;
};

struct CpuUvRasterRequest {
    std::uint32_t width;
    std::uint32_t height;
    CpuVec2f tile_origin{0.0F, 0.0F};
};

struct CpuUvRaster {
    std::uint32_t width{};
    std::uint32_t height{};
    CpuVec2f tile_origin{};
    // Projected camera depth and top-left-origin viewport coordinates per covered texel.
    std::vector<float> depth;
    std::vector<CpuVec2f> screen_position;
    std::vector<std::uint8_t> coverage;
    std::vector<std::uint32_t> triangle;
};

class CpuReferenceExecutor;

class CpuOperation {
public:
    virtual ~CpuOperation() = default;
    [[nodiscard]] virtual std::string_view identifier() const noexcept = 0;
    virtual void execute(const CpuReferenceExecutor& executor) = 0;
};

struct CpuWorkPlan {
    std::size_t work_item_count{};
    std::size_t shared_working_memory_bytes{};
    std::size_t working_memory_bytes_per_worker{};
};

class CpuBoundedOperation {
public:
    virtual ~CpuBoundedOperation() = default;
    [[nodiscard]] virtual std::string_view identifier() const noexcept = 0;
    [[nodiscard]] virtual CpuWorkPlan plan() const = 0;
    virtual void execute_work_item(const CpuReferenceExecutor& executor, std::size_t work_item,
                                   std::span<std::byte> shared_working_memory,
                                   std::span<std::byte> worker_working_memory) = 0;
    virtual void commit(std::span<const std::byte> shared_working_memory) noexcept = 0;
};

struct CpuExecutionRecord {
    std::string operation;
    bool completed;
};

class CpuReferenceExecutor final : public Executor {
public:
    CpuReferenceExecutor();

    [[nodiscard]] const ExecutorDescriptor& descriptor() const noexcept override;
    [[nodiscard]] CpuExecutionRecord execute(CpuOperation& operation) const;
    [[nodiscard]] ExecutionOutcome execute_bounded(CpuBoundedOperation& operation,
                                                   ExecutionControl control = {}) const;
    [[nodiscard]] CpuViewportRaster rasterize_viewport(const CpuRasterMeshView& mesh,
                                                       const CpuRasterCamera& camera) const;
    [[nodiscard]] CpuUvRaster rasterize_uv(const CpuRasterMeshView& mesh,
                                           const CpuRasterCamera& camera,
                                           CpuUvRasterRequest request) const;

private:
    ExecutorDescriptor descriptor_;
};

}  // namespace ctex::exec

#endif
