#ifndef CTEX_CAPI_INTERNAL_HPP
#define CTEX_CAPI_INTERNAL_HPP

#include <ctex/capi.h>

#include <ctex/doc/document.hpp>
#include <ctex/image/cube_lut.hpp>
#include <ctex/mesh/mesh.hpp>
#include <memory_resource>
#include <optional>

struct ctex_allocator_state {
    ctex_allocate_callback allocate{};
    ctex_deallocate_callback deallocate{};
    void* user_data{};
};

class ctex_host_memory_resource final : public std::pmr::memory_resource {
public:
    explicit ctex_host_memory_resource(ctex_allocator_state allocator) : allocator_(allocator) {}

private:
    void* do_allocate(std::size_t bytes, std::size_t alignment) override;
    void do_deallocate(void* allocation, std::size_t bytes, std::size_t alignment) override;
    [[nodiscard]] bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override;

    ctex_allocator_state allocator_;
};

struct ctex_document {
    explicit ctex_document(ctex_allocator_state allocator_value);

    ctex_allocator_state allocator;
    ctex_host_memory_resource memory_resource;
    ctex::doc::TextureDocument value;
};

struct ctex_cube_lut {
    ctex_cube_lut(ctex_allocator_state allocator_value, std::string_view source);

    ctex_allocator_state allocator;
    ctex_host_memory_resource memory_resource;
    ctex::image::CubeLut value;
};

struct ctex_mesh_state {
    explicit ctex_mesh_state(const ctex_mesh_descriptor& descriptor,
                             std::pmr::memory_resource* memory_resource);

    std::pmr::vector<ctex::mesh::Vec3f> positions;
    std::pmr::vector<ctex::mesh::Vec3f> normals;
    std::pmr::vector<ctex::mesh::Vec4f> vertex_colors;
    std::pmr::vector<std::uint32_t> triangle_indices;
    std::pmr::vector<ctex::mesh::Vec2f> uv_values;
    std::pmr::vector<std::pmr::string> uv_names;
    std::pmr::string default_uv_set;
    std::pmr::vector<ctex::mesh::UvSetView> uv_views;
    std::pmr::vector<std::pmr::string> partition_keys;
    std::pmr::vector<std::pmr::string> partition_names;
    std::pmr::vector<ctex::mesh::MeshPartition> partition_views;
    std::pmr::vector<std::uint32_t> face_partition_indices;
    std::pmr::vector<std::uint32_t> face_material_ids;
    std::optional<ctex::mesh::MeshBinding> binding;

    [[nodiscard]] ctex::mesh::MeshDescriptor descriptor() const noexcept;
    [[nodiscard]] const ctex::mesh::MeshBinding& mesh_binding() const noexcept { return *binding; }
};

struct ctex_mesh {
    explicit ctex_mesh(ctex_allocator_state allocator_value,
                       const ctex_mesh_descriptor& descriptor);
    ~ctex_mesh();

    ctex_allocator_state allocator;
    ctex_host_memory_resource memory_resource;
    ctex_mesh_state* state;
};

#endif
