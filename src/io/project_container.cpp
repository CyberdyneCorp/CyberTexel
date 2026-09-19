#include <lodepng.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <ctex/io/project_container.hpp>
#include <fstream>
#include <limits>
#include <memory>
#include <set>
#include <string_view>
#include <system_error>
#include <utility>

#if defined(_WIN32)
#define NOMINMAX
#include <windows.h>
#else
#include <fcntl.h>
#include <unistd.h>
#endif

namespace ctex::io {
namespace {

constexpr std::array<std::byte, 8> container_magic{std::byte{'C'}, std::byte{'T'}, std::byte{'E'},
                                                   std::byte{'X'}, std::byte{'P'}, std::byte{'R'},
                                                   std::byte{'J'}, std::byte{0}};
constexpr std::uint32_t base_header_bytes = 40;

using LodePngBuffer = std::unique_ptr<unsigned char, decltype(&std::free)>;

class ReadBudget {
public:
    explicit ReadBudget(std::size_t maximum) : maximum_(maximum) {}

    void charge(std::size_t bytes, std::string_view field) {
        if (bytes > maximum_ - std::min(maximum_, used_)) {
            throw ProjectContainerError(
                ProjectContainerErrorCode::over_limit,
                std::string(field) + " exceeds the configured aggregate allocation limit");
        }
        used_ += bytes;
    }

private:
    std::size_t maximum_{};
    std::size_t used_{};
};

class ByteWriter {
public:
    void u8(std::uint8_t value) { bytes_.push_back(static_cast<std::byte>(value)); }

    void u16(std::uint16_t value) {
        for (unsigned shift = 0; shift < 16; shift += 8) {
            u8(static_cast<std::uint8_t>(value >> shift));
        }
    }

    void u32(std::uint32_t value) {
        for (unsigned shift = 0; shift < 32; shift += 8) {
            u8(static_cast<std::uint8_t>(value >> shift));
        }
    }

    void u64(std::uint64_t value) {
        for (unsigned shift = 0; shift < 64; shift += 8) {
            u8(static_cast<std::uint8_t>(value >> shift));
        }
    }

    void bytes(std::span<const std::byte> value) {
        bytes_.insert(bytes_.end(), value.begin(), value.end());
    }

    void string(std::string_view value) {
        if (value.size() > std::numeric_limits<std::uint32_t>::max()) {
            throw ProjectContainerError(ProjectContainerErrorCode::over_limit,
                                        "container string exceeds the format limit");
        }
        u32(static_cast<std::uint32_t>(value.size()));
        bytes(std::as_bytes(std::span(value)));
    }

    [[nodiscard]] const std::vector<std::byte>& view() const noexcept { return bytes_; }
    [[nodiscard]] std::vector<std::byte> finish() && { return std::move(bytes_); }

private:
    std::vector<std::byte> bytes_;
};

class ByteReader {
public:
    explicit ByteReader(std::span<const std::byte> bytes, ReadBudget* budget = nullptr)
        : bytes_(bytes), budget_(budget) {}

    [[nodiscard]] std::uint8_t u8(std::string_view field) {
        return std::to_integer<std::uint8_t>(take(1, field)[0]);
    }

    [[nodiscard]] std::uint16_t u16(std::string_view field) {
        const auto value = take(2, field);
        return static_cast<std::uint16_t>(std::to_integer<std::uint8_t>(value[0])) |
               static_cast<std::uint16_t>(std::to_integer<std::uint8_t>(value[1]) << 8U);
    }

    [[nodiscard]] std::uint32_t u32(std::string_view field) {
        const auto value = take(4, field);
        std::uint32_t result = 0;
        for (unsigned index = 0; index < 4; ++index) {
            result |= static_cast<std::uint32_t>(std::to_integer<std::uint8_t>(value[index]))
                      << (index * 8U);
        }
        return result;
    }

    [[nodiscard]] std::uint64_t u64(std::string_view field) {
        const auto value = take(8, field);
        std::uint64_t result = 0;
        for (unsigned index = 0; index < 8; ++index) {
            result |= static_cast<std::uint64_t>(std::to_integer<std::uint8_t>(value[index]))
                      << (index * 8U);
        }
        return result;
    }

    [[nodiscard]] std::span<const std::byte> take(std::size_t size, std::string_view field) {
        if (size > bytes_.size() - offset_) {
            throw ProjectContainerError(ProjectContainerErrorCode::malformed_section,
                                        "container ends inside " + std::string(field));
        }
        const auto result = bytes_.subspan(offset_, size);
        offset_ += size;
        return result;
    }

    [[nodiscard]] std::string string(std::size_t maximum, std::string_view field) {
        const std::uint32_t size = u32(field);
        if (size > maximum) {
            throw ProjectContainerError(
                ProjectContainerErrorCode::over_limit,
                std::string(field) + " exceeds the configured string limit");
        }
        const auto value = take(size, field);
        if (budget_ != nullptr) {
            budget_->charge(size, field);
        }
        return {reinterpret_cast<const char*>(value.data()), value.size()};
    }

    [[nodiscard]] bool empty() const noexcept { return offset_ == bytes_.size(); }
    [[nodiscard]] std::size_t remaining() const noexcept { return bytes_.size() - offset_; }

private:
    std::span<const std::byte> bytes_;
    std::size_t offset_{};
    ReadBudget* budget_{};
};

struct ContainerHeader {
    ContainerSchemaVersion version;
    std::uint32_t header_bytes{};
    std::uint32_t section_count{};
    std::uint64_t body_bytes{};
};

struct UnsupportedTileSection : std::runtime_error {
    using std::runtime_error::runtime_error;
};

struct UnsupportedResourceSection : std::runtime_error {
    using std::runtime_error::runtime_error;
};

[[noreturn]] void throw_filesystem_error(std::string_view operation, int error) {
    const std::error_code code(error, std::system_category());
    throw ProjectContainerError(ProjectContainerErrorCode::filesystem_failure,
                                std::string(operation) + ": " + code.message());
}

#if defined(_WIN32)
using NativeFile = HANDLE;
constexpr NativeFile invalid_native_file = INVALID_HANDLE_VALUE;

std::uint64_t process_identity() noexcept { return GetCurrentProcessId(); }

NativeFile create_exclusive_file(const std::filesystem::path& path) {
    return CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL,
                       nullptr);
}

void close_file(NativeFile file) noexcept { CloseHandle(file); }

void write_and_sync_file(NativeFile file, std::span<const std::byte> bytes) {
    std::size_t offset = 0;
    while (offset < bytes.size()) {
        const std::size_t remaining = bytes.size() - offset;
        const DWORD request =
            static_cast<DWORD>(std::min<std::size_t>(remaining, std::numeric_limits<DWORD>::max()));
        DWORD written = 0;
        if (!WriteFile(file, bytes.data() + offset, request, &written, nullptr) || written == 0) {
            throw_filesystem_error("could not write project temporary file", GetLastError());
        }
        offset += written;
    }
    if (!FlushFileBuffers(file)) {
        throw_filesystem_error("could not synchronize project temporary file", GetLastError());
    }
}

void replace_file(const std::filesystem::path& temporary, const std::filesystem::path& target) {
    if (!MoveFileExW(temporary.c_str(), target.c_str(),
                     MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        throw_filesystem_error("could not atomically publish project file", GetLastError());
    }
}

void sync_directory(const std::filesystem::path&) {}
#else
using NativeFile = int;
constexpr NativeFile invalid_native_file = -1;

std::uint64_t process_identity() noexcept { return static_cast<std::uint64_t>(::getpid()); }

NativeFile create_exclusive_file(const std::filesystem::path& path) {
    return ::open(path.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, 0666);
}

void close_file(NativeFile file) noexcept { static_cast<void>(::close(file)); }

void write_and_sync_file(NativeFile file, std::span<const std::byte> bytes) {
    std::size_t offset = 0;
    while (offset < bytes.size()) {
        const std::size_t request = std::min(
            bytes.size() - offset, static_cast<std::size_t>(std::numeric_limits<ssize_t>::max()));
        const ssize_t written = ::write(file, bytes.data() + offset, request);
        if (written < 0 && errno == EINTR) {
            continue;
        }
        if (written <= 0) {
            throw_filesystem_error("could not write project temporary file", errno);
        }
        offset += static_cast<std::size_t>(written);
    }
    if (::fsync(file) != 0) {
        throw_filesystem_error("could not synchronize project temporary file", errno);
    }
}

void replace_file(const std::filesystem::path& temporary, const std::filesystem::path& target) {
    if (::rename(temporary.c_str(), target.c_str()) != 0) {
        throw_filesystem_error("could not atomically publish project file", errno);
    }
}

void sync_directory(const std::filesystem::path& directory) {
    const int descriptor = ::open(directory.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (descriptor < 0) {
        throw_filesystem_error("could not open project directory for synchronization", errno);
    }
    if (::fsync(descriptor) != 0) {
        const int error = errno;
        close_file(descriptor);
        throw_filesystem_error("could not synchronize project directory", error);
    }
    close_file(descriptor);
}
#endif

class TemporaryProjectFile {
public:
    explicit TemporaryProjectFile(const std::filesystem::path& target) {
        static std::atomic_uint64_t next_identity{1};
        const std::filesystem::path directory =
            target.has_parent_path() ? target.parent_path() : std::filesystem::path{"."};
        std::filesystem::path prefix{"."};
        prefix += target.filename();
        prefix += ".tmp.";
        prefix += std::to_string(process_identity());
        prefix += ".";
        for (unsigned attempt = 0; attempt < 128; ++attempt) {
            std::filesystem::path name = prefix;
            name += std::to_string(next_identity.fetch_add(1));
            path_ = directory / name;
            file_ = create_exclusive_file(path_);
            if (file_ != invalid_native_file) {
                return;
            }
#if defined(_WIN32)
            const int error = static_cast<int>(GetLastError());
            if (error != ERROR_FILE_EXISTS && error != ERROR_ALREADY_EXISTS) {
                throw_filesystem_error("could not create project temporary file", error);
            }
#else
            if (errno != EEXIST) {
                throw_filesystem_error("could not create project temporary file", errno);
            }
#endif
        }
        throw ProjectContainerError(ProjectContainerErrorCode::filesystem_failure,
                                    "could not allocate a unique project temporary file");
    }

    TemporaryProjectFile(const TemporaryProjectFile&) = delete;
    TemporaryProjectFile& operator=(const TemporaryProjectFile&) = delete;

    ~TemporaryProjectFile() {
        if (file_ != invalid_native_file) {
            close_file(file_);
        }
        if (!published_) {
            std::error_code ignored;
            std::filesystem::remove(path_, ignored);
        }
    }

    void write_and_sync(std::span<const std::byte> bytes) {
        write_and_sync_file(file_, bytes);
        close_file(file_);
        file_ = invalid_native_file;
    }

    void publish(const std::filesystem::path& target) {
        replace_file(path_, target);
        published_ = true;
        const std::filesystem::path directory =
            target.has_parent_path() ? target.parent_path() : std::filesystem::path{"."};
        sync_directory(directory);
    }

private:
    std::filesystem::path path_;
    NativeFile file_{invalid_native_file};
    bool published_{};
};

std::size_t checked_size(std::uint64_t value, std::string_view field) {
    if (value > std::numeric_limits<std::size_t>::max()) {
        throw ProjectContainerError(ProjectContainerErrorCode::over_limit,
                                    std::string(field) + " exceeds the platform size limit");
    }
    return static_cast<std::size_t>(value);
}

std::size_t checked_multiply(std::size_t left, std::size_t right, std::string_view field) {
    if (right != 0 && left > std::numeric_limits<std::size_t>::max() / right) {
        throw ProjectContainerError(ProjectContainerErrorCode::over_limit,
                                    std::string(field) + " overflows the platform size limit");
    }
    return left * right;
}

void require_records_fit(std::size_t count, std::size_t remaining, std::size_t minimum_record_bytes,
                         std::string_view field) {
    if (count > remaining / minimum_record_bytes) {
        throw ProjectContainerError(ProjectContainerErrorCode::malformed_section,
                                    std::string(field) + " cannot fit in its declared payload");
    }
}

void add_to_total(std::size_t count, std::size_t maximum, std::size_t& total,
                  std::string_view field) {
    if (count > maximum - std::min(maximum, total)) {
        throw ProjectContainerError(ProjectContainerErrorCode::over_limit,
                                    std::string(field) + " exceeds the configured limit");
    }
    total += count;
}

void charge_identity(ReadBudget& budget, std::string_view identity, std::string_view field) {
    constexpr std::size_t set_node_overhead = sizeof(std::string) + 3 * sizeof(void*);
    budget.charge(set_node_overhead, field);
    budget.charge(identity.size(), field);
}

ContainerHeader read_header(std::span<const std::byte> bytes, bool require_body) {
    if (bytes.size() < base_header_bytes ||
        !std::equal(container_magic.begin(), container_magic.end(), bytes.begin())) {
        throw ProjectContainerError(ProjectContainerErrorCode::malformed_header,
                                    "project container header or magic is invalid");
    }
    ByteReader reader(bytes.subspan(container_magic.size()));
    const std::uint32_t header_bytes = reader.u32("header size");
    const ContainerSchemaVersion version{reader.u32("schema major"), reader.u32("schema minor"),
                                         reader.u32("schema patch")};
    const std::uint32_t section_count = reader.u32("section count");
    static_cast<void>(reader.u32("header reserved field"));
    const std::uint64_t body_bytes = reader.u64("body size");
    if (header_bytes < base_header_bytes || header_bytes > bytes.size()) {
        throw ProjectContainerError(ProjectContainerErrorCode::malformed_header,
                                    "project container header size is invalid");
    }
    if (require_body && body_bytes != bytes.size() - header_bytes) {
        throw ProjectContainerError(ProjectContainerErrorCode::malformed_header,
                                    "project container body size does not match the file");
    }
    return {.version = version,
            .header_bytes = header_bytes,
            .section_count = section_count,
            .body_bytes = body_bytes};
}

bool newer_than_current(ContainerSchemaVersion version) noexcept {
    if (version.major != current_container_schema.major) {
        return version.major > current_container_schema.major;
    }
    if (version.minor != current_container_schema.minor) {
        return version.minor > current_container_schema.minor;
    }
    return version.patch > current_container_schema.patch;
}

std::vector<std::byte> compress_tile(std::span<const std::byte> pixels) {
    unsigned char* encoded = nullptr;
    std::size_t encoded_size = 0;
    const unsigned error = lodepng_zlib_compress(
        &encoded, &encoded_size, reinterpret_cast<const unsigned char*>(pixels.data()),
        pixels.size(), &lodepng_default_compress_settings);
    LodePngBuffer owner(encoded, &std::free);
    if (error != 0) {
        throw ProjectContainerError(
            ProjectContainerErrorCode::compression_failed,
            "tile compression failed: " + std::string(lodepng_error_text(error)));
    }
    const auto* begin = reinterpret_cast<const std::byte*>(owner.get());
    return {begin, begin + encoded_size};
}

std::vector<std::byte> decompress_tile(std::span<const std::byte> encoded,
                                       std::size_t expected_size, std::size_t maximum_size) {
    if (expected_size > maximum_size) {
        throw ProjectContainerError(ProjectContainerErrorCode::over_limit,
                                    "decoded tile exceeds the configured byte limit");
    }
    LodePNGDecompressSettings settings = lodepng_default_decompress_settings;
    settings.max_output_size = expected_size;
    unsigned char* decoded = nullptr;
    std::size_t decoded_size = 0;
    const unsigned error = lodepng_zlib_decompress(
        &decoded, &decoded_size, reinterpret_cast<const unsigned char*>(encoded.data()),
        encoded.size(), &settings);
    LodePngBuffer owner(decoded, &std::free);
    if (error != 0 || decoded_size != expected_size) {
        throw ProjectContainerError(ProjectContainerErrorCode::compression_failed,
                                    "tile decompression failed or produced the wrong byte count");
    }
    const auto* begin = reinterpret_cast<const std::byte*>(owner.get());
    return {begin, begin + decoded_size};
}

image::TileExtent expected_extent(const StoredTiledImage& image, image::TileCoordinate coordinate) {
    const std::uint32_t columns = 1 + ((image.width - 1) / image.tile_size);
    const std::uint32_t rows = 1 + ((image.height - 1) / image.tile_size);
    if (coordinate.x >= columns || coordinate.y >= rows) {
        throw ProjectContainerError(ProjectContainerErrorCode::invalid_tile,
                                    "stored tile coordinate is outside its image");
    }
    return {.width = std::min(image.tile_size, image.width - coordinate.x * image.tile_size),
            .height = std::min(image.tile_size, image.height - coordinate.y * image.tile_size)};
}

void validate_image(const StoredTiledImage& image, ReadBudget* budget = nullptr) {
    if (image.resource_id.empty() || image.width == 0 || image.height == 0 ||
        image.tile_size == 0 || !image.format.is_valid() ||
        image.clear_pixel.size() != image.format.bytes_per_pixel()) {
        throw ProjectContainerError(ProjectContainerErrorCode::invalid_tile,
                                    "stored tiled image metadata is invalid");
    }
    std::set<std::uint64_t> coordinates;
    for (const StoredTile& tile : image.occupied_tiles) {
        const image::TileExtent extent = expected_extent(image, tile.coordinate);
        const std::size_t expected_bytes =
            checked_multiply(checked_multiply(extent.width, extent.height, "stored tile area"),
                             image.format.bytes_per_pixel(), "stored tile bytes");
        const std::uint64_t key =
            (static_cast<std::uint64_t>(tile.coordinate.y) << 32U) | tile.coordinate.x;
        if (budget != nullptr) {
            budget->charge(sizeof(std::uint64_t) + 3 * sizeof(void*),
                           "tile coordinate identity index");
        }
        if (tile.extent.width != extent.width || tile.extent.height != extent.height ||
            tile.pixels.size() != expected_bytes ||
            tile.compression != TileCompression::zlib_deflate || !coordinates.insert(key).second) {
            throw ProjectContainerError(
                ProjectContainerErrorCode::invalid_tile,
                "stored tile extent, bytes, compression or identity is invalid");
        }
    }
}

std::vector<std::byte> encode_tiled_images(std::span<const StoredTiledImage> images) {
    if (images.size() > std::numeric_limits<std::uint32_t>::max()) {
        throw ProjectContainerError(ProjectContainerErrorCode::over_limit,
                                    "container image count exceeds the format limit");
    }
    ByteWriter writer;
    writer.u32(static_cast<std::uint32_t>(images.size()));
    std::set<std::string_view> identities;
    for (const StoredTiledImage& image : images) {
        validate_image(image);
        if (!identities.insert(image.resource_id).second) {
            throw ProjectContainerError(ProjectContainerErrorCode::invalid_tile,
                                        "container repeats a tiled image resource identity");
        }
        writer.string(image.resource_id);
        writer.u32(image.width);
        writer.u32(image.height);
        writer.u32(image.tile_size);
        writer.u8(static_cast<std::uint8_t>(image.format.channel_type));
        writer.u8(image.format.channel_count);
        writer.u16(static_cast<std::uint16_t>(image.clear_pixel.size()));
        writer.bytes(image.clear_pixel);
        if (image.occupied_tiles.size() > std::numeric_limits<std::uint32_t>::max()) {
            throw ProjectContainerError(ProjectContainerErrorCode::over_limit,
                                        "occupied tile count exceeds the format limit");
        }
        writer.u32(static_cast<std::uint32_t>(image.occupied_tiles.size()));
        for (const StoredTile& tile : image.occupied_tiles) {
            const std::vector<std::byte> compressed = compress_tile(tile.pixels);
            writer.u32(tile.coordinate.x);
            writer.u32(tile.coordinate.y);
            writer.u32(tile.extent.width);
            writer.u32(tile.extent.height);
            writer.u8(static_cast<std::uint8_t>(tile.compression));
            writer.u8(0);
            writer.u16(0);
            writer.u64(tile.pixels.size());
            writer.u64(compressed.size());
            writer.bytes(compressed);
        }
    }
    return std::move(writer).finish();
}

image::PixelFormat read_pixel_format(ByteReader& reader) {
    const std::uint8_t channel_type = reader.u8("tile channel type");
    const std::uint8_t channel_count = reader.u8("tile channel count");
    if (channel_type > static_cast<std::uint8_t>(image::ChannelType::float32)) {
        throw UnsupportedTileSection("unknown tile channel type");
    }
    if (channel_count > 4) {
        throw UnsupportedTileSection("unknown tile channel count");
    }
    const image::PixelFormat format{static_cast<image::ChannelType>(channel_type), channel_count};
    if (!format.is_valid()) {
        throw ProjectContainerError(ProjectContainerErrorCode::invalid_tile,
                                    "stored tiled image pixel format is invalid");
    }
    return format;
}

StoredTile decode_tile(ByteReader& reader, const StoredTiledImage& image,
                       const ProjectContainerReadLimits& limits, ReadBudget& budget) {
    StoredTile tile;
    tile.coordinate = {reader.u32("tile x"), reader.u32("tile y")};
    tile.extent = {reader.u32("tile width"), reader.u32("tile height")};
    const std::uint8_t compression = reader.u8("tile compression");
    static_cast<void>(reader.u8("tile reserved byte"));
    static_cast<void>(reader.u16("tile reserved bytes"));
    if (compression != static_cast<std::uint8_t>(TileCompression::zlib_deflate)) {
        throw UnsupportedTileSection("unknown tile compression");
    }
    tile.compression = static_cast<TileCompression>(compression);
    const std::size_t raw_size = checked_size(reader.u64("tile decoded size"), "tile decoded size");
    const std::size_t encoded_size =
        checked_size(reader.u64("tile encoded size"), "tile encoded size");
    const auto encoded = reader.take(encoded_size, "compressed tile bytes");
    const image::TileExtent extent = expected_extent(image, tile.coordinate);
    const std::size_t expected_size =
        checked_multiply(checked_multiply(extent.width, extent.height, "decoded tile area"),
                         image.format.bytes_per_pixel(), "decoded tile bytes");
    if (tile.extent.width != extent.width || tile.extent.height != extent.height ||
        raw_size != expected_size) {
        throw ProjectContainerError(ProjectContainerErrorCode::invalid_tile,
                                    "stored tile declares an invalid extent or decoded byte count");
    }
    budget.charge(raw_size, "decoded tile bytes");
    tile.pixels = decompress_tile(encoded, raw_size, limits.maximum_decoded_tile_bytes);
    return tile;
}

std::vector<StoredTiledImage> decode_tiled_images(std::span<const std::byte> payload,
                                                  const ProjectContainerReadLimits& limits,
                                                  std::size_t& total_images,
                                                  std::size_t& total_tiles, ReadBudget& budget) {
    ByteReader reader(payload, &budget);
    const std::uint32_t image_count = reader.u32("tiled image count");
    add_to_total(image_count, limits.maximum_images, total_images, "tiled image count");
    require_records_fit(image_count, reader.remaining(), 24, "tiled image count");
    budget.charge(checked_multiply(image_count, sizeof(StoredTiledImage), "tiled image metadata"),
                  "tiled image metadata");
    std::vector<StoredTiledImage> images;
    images.reserve(image_count);
    std::set<std::string> identities;
    for (std::uint32_t index = 0; index < image_count; ++index) {
        StoredTiledImage image;
        image.resource_id = reader.string(limits.maximum_string_bytes, "tiled image identity");
        image.width = reader.u32("tiled image width");
        image.height = reader.u32("tiled image height");
        image.tile_size = reader.u32("tiled image tile size");
        image.format = read_pixel_format(reader);
        const std::uint16_t clear_size = reader.u16("clear pixel byte count");
        const auto clear_pixel = reader.take(clear_size, "clear pixel");
        budget.charge(clear_size, "clear pixel bytes");
        image.clear_pixel.assign(clear_pixel.begin(), clear_pixel.end());
        validate_image(image);
        const std::uint32_t tile_count = reader.u32("occupied tile count");
        add_to_total(tile_count, limits.maximum_tiles, total_tiles, "occupied tile count");
        require_records_fit(tile_count, reader.remaining(), 36, "occupied tile count");
        budget.charge(checked_multiply(tile_count, sizeof(StoredTile), "tile metadata"),
                      "tile metadata");
        image.occupied_tiles.reserve(tile_count);
        for (std::uint32_t tile_index = 0; tile_index < tile_count; ++tile_index) {
            image.occupied_tiles.push_back(decode_tile(reader, image, limits, budget));
        }
        validate_image(image, &budget);
        charge_identity(budget, image.resource_id, "tiled image identity index");
        if (!identities.insert(image.resource_id).second) {
            throw ProjectContainerError(ProjectContainerErrorCode::invalid_tile,
                                        "container repeats a tiled image resource identity");
        }
        images.push_back(std::move(image));
    }
    if (!reader.empty()) {
        throw ProjectContainerError(ProjectContainerErrorCode::malformed_section,
                                    "tiled pixel section has trailing bytes");
    }
    return images;
}

void validate_resource(const ProjectResource& resource) {
    const std::filesystem::path path(resource.relative_path);
    if (resource.identifier.empty() || resource.kind.empty() || resource.relative_path.empty() ||
        path.is_absolute() || path.has_root_name() || path.has_root_directory()) {
        throw ProjectContainerError(ProjectContainerErrorCode::invalid_resource,
                                    "project resource metadata or path is invalid");
    }
    for (const auto& component : path) {
        if (component == "..") {
            throw ProjectContainerError(ProjectContainerErrorCode::invalid_resource,
                                        "project resource path escapes the project directory");
        }
    }
}

std::vector<std::byte> encode_resources(std::span<const ProjectResource> resources) {
    if (resources.size() > std::numeric_limits<std::uint32_t>::max()) {
        throw ProjectContainerError(ProjectContainerErrorCode::over_limit,
                                    "container resource count exceeds the format limit");
    }
    ByteWriter writer;
    writer.u32(static_cast<std::uint32_t>(resources.size()));
    std::set<std::string_view> identities;
    for (const ProjectResource& resource : resources) {
        validate_resource(resource);
        if (!identities.insert(resource.identifier).second) {
            throw ProjectContainerError(ProjectContainerErrorCode::invalid_resource,
                                        "container repeats a project resource identity");
        }
        writer.string(resource.identifier);
        writer.string(resource.kind);
        writer.string(resource.relative_path);
        writer.u8(resource.packed_bytes.has_value() ? 1 : 0);
        writer.u8(0);
        writer.u16(0);
        const std::span<const std::byte> payload = resource.packed_bytes.has_value()
                                                       ? std::span(*resource.packed_bytes)
                                                       : std::span<const std::byte>{};
        writer.u64(payload.size());
        writer.bytes(payload);
    }
    return std::move(writer).finish();
}

ProjectResource decode_resource(ByteReader& reader, const ProjectContainerReadLimits& limits,
                                ReadBudget& budget) {
    ProjectResource resource;
    resource.identifier = reader.string(limits.maximum_string_bytes, "resource identity");
    resource.kind = reader.string(limits.maximum_string_bytes, "resource kind");
    resource.relative_path = reader.string(limits.maximum_string_bytes, "resource relative path");
    const std::uint8_t storage = reader.u8("resource storage");
    static_cast<void>(reader.u8("resource reserved byte"));
    static_cast<void>(reader.u16("resource reserved bytes"));
    const std::size_t payload_size =
        checked_size(reader.u64("resource payload size"), "resource payload size");
    if (payload_size > limits.maximum_packed_resource_bytes) {
        throw ProjectContainerError(ProjectContainerErrorCode::over_limit,
                                    "packed resource exceeds the configured byte limit");
    }
    const auto payload = reader.take(payload_size, "resource payload");
    if (storage == 0) {
        if (!payload.empty()) {
            throw ProjectContainerError(ProjectContainerErrorCode::invalid_resource,
                                        "referenced resource contains a packed payload");
        }
    } else if (storage == 1) {
        budget.charge(payload.size(), "packed resource bytes");
        resource.packed_bytes = std::vector<std::byte>(payload.begin(), payload.end());
    } else {
        throw UnsupportedResourceSection("unknown project resource storage encoding");
    }
    validate_resource(resource);
    return resource;
}

std::vector<ProjectResource> decode_resources(std::span<const std::byte> payload,
                                              const ProjectContainerReadLimits& limits,
                                              std::size_t& total_resources, ReadBudget& budget) {
    ByteReader reader(payload, &budget);
    const std::uint32_t resource_count = reader.u32("project resource count");
    add_to_total(resource_count, limits.maximum_resources, total_resources,
                 "project resource count");
    require_records_fit(resource_count, reader.remaining(), 24, "project resource count");
    budget.charge(
        checked_multiply(resource_count, sizeof(ProjectResource), "project resource metadata"),
        "project resource metadata");
    std::vector<ProjectResource> resources;
    resources.reserve(resource_count);
    std::set<std::string> identities;
    for (std::uint32_t index = 0; index < resource_count; ++index) {
        ProjectResource resource = decode_resource(reader, limits, budget);
        charge_identity(budget, resource.identifier, "project resource identity index");
        if (!identities.insert(resource.identifier).second) {
            throw ProjectContainerError(ProjectContainerErrorCode::invalid_resource,
                                        "container repeats a project resource identity");
        }
        resources.push_back(std::move(resource));
    }
    if (!reader.empty()) {
        throw ProjectContainerError(ProjectContainerErrorCode::malformed_section,
                                    "project resource section has trailing bytes");
    }
    return resources;
}

void validate_asset(const StandaloneAsset& asset, ReadBudget* budget = nullptr) {
    if (asset.identifier.empty() || asset.kind.empty() || asset.format_version == 0) {
        throw ProjectContainerError(ProjectContainerErrorCode::invalid_asset,
                                    "standalone asset metadata is invalid");
    }
    std::set<std::string_view> resource_dependencies;
    for (const std::string& dependency : asset.resource_dependencies) {
        if (budget != nullptr) {
            budget->charge(sizeof(std::string_view) + 3 * sizeof(void*),
                           "asset resource dependency identity index");
        }
        if (dependency.empty() || !resource_dependencies.insert(dependency).second) {
            throw ProjectContainerError(ProjectContainerErrorCode::invalid_asset,
                                        "standalone asset resource dependency is invalid");
        }
    }
    std::set<std::string_view> tiled_dependencies;
    for (const std::string& dependency : asset.tiled_image_dependencies) {
        if (budget != nullptr) {
            budget->charge(sizeof(std::string_view) + 3 * sizeof(void*),
                           "asset tiled dependency identity index");
        }
        if (dependency.empty() || !tiled_dependencies.insert(dependency).second) {
            throw ProjectContainerError(ProjectContainerErrorCode::invalid_asset,
                                        "standalone asset tiled dependency is invalid");
        }
    }
}

std::vector<std::byte> encode_assets(std::span<const StandaloneAsset> assets) {
    if (assets.size() > std::numeric_limits<std::uint32_t>::max()) {
        throw ProjectContainerError(ProjectContainerErrorCode::over_limit,
                                    "container asset count exceeds the format limit");
    }
    ByteWriter writer;
    writer.u32(static_cast<std::uint32_t>(assets.size()));
    std::set<std::string_view> identities;
    for (const StandaloneAsset& asset : assets) {
        validate_asset(asset);
        if (!identities.insert(asset.identifier).second ||
            asset.resource_dependencies.size() > std::numeric_limits<std::uint32_t>::max() ||
            asset.tiled_image_dependencies.size() > std::numeric_limits<std::uint32_t>::max()) {
            throw ProjectContainerError(ProjectContainerErrorCode::invalid_asset,
                                        "container asset identity or dependency count is invalid");
        }
        writer.string(asset.identifier);
        writer.string(asset.kind);
        writer.u32(asset.format_version);
        writer.u32(static_cast<std::uint32_t>(asset.resource_dependencies.size()));
        writer.u32(static_cast<std::uint32_t>(asset.tiled_image_dependencies.size()));
        writer.u64(asset.payload.size());
        for (const std::string& dependency : asset.resource_dependencies) {
            writer.string(dependency);
        }
        for (const std::string& dependency : asset.tiled_image_dependencies) {
            writer.string(dependency);
        }
        writer.bytes(asset.payload);
    }
    return std::move(writer).finish();
}

StandaloneAsset decode_asset(ByteReader& reader, const ProjectContainerReadLimits& limits,
                             std::size_t& total_dependencies, ReadBudget& budget) {
    StandaloneAsset asset;
    asset.identifier = reader.string(limits.maximum_string_bytes, "asset identity");
    asset.kind = reader.string(limits.maximum_string_bytes, "asset kind");
    asset.format_version = reader.u32("asset format version");
    const std::uint32_t resource_count = reader.u32("asset resource dependency count");
    const std::uint32_t tiled_count = reader.u32("asset tiled dependency count");
    const std::size_t payload_size =
        checked_size(reader.u64("asset payload size"), "asset payload size");
    add_to_total(resource_count, limits.maximum_asset_dependencies, total_dependencies,
                 "asset dependencies");
    add_to_total(tiled_count, limits.maximum_asset_dependencies, total_dependencies,
                 "asset dependencies");
    if (payload_size > limits.maximum_asset_payload_bytes) {
        throw ProjectContainerError(ProjectContainerErrorCode::over_limit,
                                    "asset payload exceeds the configured byte limit");
    }
    if (payload_size > reader.remaining()) {
        throw ProjectContainerError(ProjectContainerErrorCode::malformed_section,
                                    "asset payload cannot fit in its declared section");
    }
    require_records_fit(static_cast<std::size_t>(resource_count) + tiled_count,
                        reader.remaining() - payload_size, 4, "asset dependency count");
    budget.charge(
        checked_multiply(resource_count, sizeof(std::string), "asset resource dependency metadata"),
        "asset resource dependency metadata");
    budget.charge(
        checked_multiply(tiled_count, sizeof(std::string), "asset tiled dependency metadata"),
        "asset tiled dependency metadata");
    asset.resource_dependencies.reserve(resource_count);
    for (std::uint32_t index = 0; index < resource_count; ++index) {
        asset.resource_dependencies.push_back(
            reader.string(limits.maximum_string_bytes, "asset resource dependency"));
    }
    asset.tiled_image_dependencies.reserve(tiled_count);
    for (std::uint32_t index = 0; index < tiled_count; ++index) {
        asset.tiled_image_dependencies.push_back(
            reader.string(limits.maximum_string_bytes, "asset tiled dependency"));
    }
    const auto payload = reader.take(payload_size, "asset payload");
    budget.charge(payload.size(), "asset payload bytes");
    asset.payload.assign(payload.begin(), payload.end());
    validate_asset(asset, &budget);
    return asset;
}

std::vector<StandaloneAsset> decode_assets(std::span<const std::byte> payload,
                                           const ProjectContainerReadLimits& limits,
                                           std::size_t& total_assets,
                                           std::size_t& total_dependencies, ReadBudget& budget) {
    ByteReader reader(payload, &budget);
    const std::uint32_t asset_count = reader.u32("standalone asset count");
    add_to_total(asset_count, limits.maximum_assets, total_assets, "standalone asset count");
    require_records_fit(asset_count, reader.remaining(), 28, "standalone asset count");
    budget.charge(checked_multiply(asset_count, sizeof(StandaloneAsset), "asset metadata"),
                  "asset metadata");
    std::vector<StandaloneAsset> assets;
    assets.reserve(asset_count);
    std::set<std::string> identities;
    for (std::uint32_t index = 0; index < asset_count; ++index) {
        StandaloneAsset asset = decode_asset(reader, limits, total_dependencies, budget);
        charge_identity(budget, asset.identifier, "standalone asset identity index");
        if (!identities.insert(asset.identifier).second) {
            throw ProjectContainerError(ProjectContainerErrorCode::invalid_asset,
                                        "container repeats a standalone asset identity");
        }
        assets.push_back(std::move(asset));
    }
    if (!reader.empty()) {
        throw ProjectContainerError(ProjectContainerErrorCode::malformed_section,
                                    "standalone asset section has trailing bytes");
    }
    return assets;
}

void validate_asset_dependencies(const ProjectContainer& container,
                                 const std::set<std::string>* known_resources = nullptr,
                                 const std::set<std::string>* known_tiles = nullptr) {
    const bool has_opaque_resources =
        std::any_of(container.opaque_sections.begin(), container.opaque_sections.end(),
                    [](const OpaqueContainerSection& section) {
                        return section.kind == project_resource_section_kind;
                    });
    const bool has_opaque_tiles =
        std::any_of(container.opaque_sections.begin(), container.opaque_sections.end(),
                    [](const OpaqueContainerSection& section) {
                        return section.kind == tiled_pixel_section_kind;
                    });
    std::set<std::string_view> resource_identities;
    if (known_resources == nullptr) {
        for (const ProjectResource& resource : container.resources) {
            resource_identities.insert(resource.identifier);
        }
    }
    std::set<std::string_view> tiled_identities;
    if (known_tiles == nullptr) {
        for (const StoredTiledImage& image : container.tiled_images) {
            tiled_identities.insert(image.resource_id);
        }
    }
    for (const StandaloneAsset& asset : container.assets) {
        for (const std::string& dependency : asset.resource_dependencies) {
            const bool present = known_resources != nullptr
                                     ? known_resources->contains(dependency)
                                     : resource_identities.contains(dependency);
            if (!present && !has_opaque_resources) {
                throw ProjectContainerError(
                    ProjectContainerErrorCode::invalid_asset,
                    "standalone asset resource dependency is absent: " + dependency);
            }
        }
        for (const std::string& dependency : asset.tiled_image_dependencies) {
            const bool present = known_tiles != nullptr ? known_tiles->contains(dependency)
                                                        : tiled_identities.contains(dependency);
            if (!present && !has_opaque_tiles) {
                throw ProjectContainerError(
                    ProjectContainerErrorCode::invalid_asset,
                    "standalone asset tiled dependency is absent: " + dependency);
            }
        }
    }
}

void append_section(ByteWriter& body, std::uint32_t kind, std::uint32_t version,
                    std::span<const std::byte> payload) {
    body.u32(kind);
    body.u32(version);
    body.u64(payload.size());
    body.bytes(payload);
}

void append_opaque_section(ProjectContainerReadResult& result, std::uint32_t kind,
                           std::uint32_t version, std::span<const std::byte> payload,
                           std::string_view message, ReadBudget& budget) {
    budget.charge(sizeof(OpaqueContainerSection) + sizeof(UnknownContainerPart),
                  "opaque section metadata");
    budget.charge(payload.size(), "opaque section payload");
    budget.charge(message.size(), "opaque section diagnostic");
    result.report.unknown_parts.push_back({.section_kind = kind,
                                           .section_version = version,
                                           .payload_bytes = payload.size(),
                                           .message = std::string(message)});
    result.container.opaque_sections.push_back(
        {.kind = kind, .version = version, .payload = {payload.begin(), payload.end()}});
}

}  // namespace

ProjectContainerError::ProjectContainerError(ProjectContainerErrorCode code, std::string message)
    : std::runtime_error(std::move(message)), code_(code) {}

ContainerSchemaVersion probe_project_container_version(std::span<const std::byte> bytes) {
    return read_header(bytes, false).version;
}

std::vector<std::byte> write_project_container(const ProjectContainer& container) {
    if (container.opaque_sections.size() >
        static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max()) - 3) {
        throw ProjectContainerError(ProjectContainerErrorCode::over_limit,
                                    "container section count exceeds the format limit");
    }
    const std::vector<std::byte> tiled_payload = encode_tiled_images(container.tiled_images);
    const std::vector<std::byte> resource_payload = encode_resources(container.resources);
    const std::vector<std::byte> asset_payload = encode_assets(container.assets);
    validate_asset_dependencies(container);
    ByteWriter body;
    append_section(body, tiled_pixel_section_kind, tiled_pixel_section_version, tiled_payload);
    append_section(body, project_resource_section_kind, project_resource_section_version,
                   resource_payload);
    append_section(body, standalone_asset_section_kind, standalone_asset_section_version,
                   asset_payload);
    for (const OpaqueContainerSection& section : container.opaque_sections) {
        append_section(body, section.kind, section.version, section.payload);
    }

    ByteWriter writer;
    writer.bytes(container_magic);
    writer.u32(base_header_bytes);
    writer.u32(container.schema_version.major);
    writer.u32(container.schema_version.minor);
    writer.u32(container.schema_version.patch);
    writer.u32(static_cast<std::uint32_t>(container.opaque_sections.size() + 3));
    writer.u32(0);
    writer.u64(body.view().size());
    writer.bytes(body.view());
    return std::move(writer).finish();
}

void save_project_container_atomic(const std::filesystem::path& path,
                                   const ProjectContainer& container) {
    if (path.empty() || path.filename().empty()) {
        throw ProjectContainerError(ProjectContainerErrorCode::filesystem_failure,
                                    "project save path must name a file");
    }
    const std::vector<std::byte> bytes = write_project_container(container);
    TemporaryProjectFile temporary(path);
    temporary.write_and_sync(bytes);
    temporary.publish(path);
}

ProjectContainerReadResult read_project_container(std::span<const std::byte> bytes,
                                                  ProjectContainerReadLimits limits) {
    if (bytes.size() > limits.maximum_input_bytes) {
        throw ProjectContainerError(ProjectContainerErrorCode::over_limit,
                                    "project container exceeds the configured input byte limit");
    }
    const ContainerHeader header = read_header(bytes, true);
    if (header.section_count > limits.maximum_sections) {
        throw ProjectContainerError(ProjectContainerErrorCode::over_limit,
                                    "container section count exceeds the configured limit");
    }
    ProjectContainerReadResult result{.container = {.schema_version = header.version,
                                                    .tiled_images = {},
                                                    .resources = {},
                                                    .assets = {},
                                                    .opaque_sections = {}},
                                      .report = {.source_schema = header.version,
                                                 .newer_schema = newer_than_current(header.version),
                                                 .unknown_parts = {}}};
    ReadBudget budget(limits.maximum_total_allocation_bytes);
    ByteReader body(
        bytes.subspan(header.header_bytes, checked_size(header.body_bytes, "body size")));
    require_records_fit(header.section_count, body.remaining(), 16, "container section count");
    std::size_t total_images = 0;
    std::size_t total_tiles = 0;
    std::size_t total_resources = 0;
    std::set<std::string> image_identities;
    std::set<std::string> resource_identities;
    std::set<std::string> asset_identities;
    std::size_t total_assets = 0;
    std::size_t total_asset_dependencies = 0;
    for (std::uint32_t index = 0; index < header.section_count; ++index) {
        const std::uint32_t kind = body.u32("section kind");
        const std::uint32_t version = body.u32("section version");
        const std::size_t payload_size =
            checked_size(body.u64("section payload size"), "section payload size");
        const auto payload = body.take(payload_size, "section payload");
        if (kind == tiled_pixel_section_kind && version == tiled_pixel_section_version) {
            try {
                auto images =
                    decode_tiled_images(payload, limits, total_images, total_tiles, budget);
                for (StoredTiledImage& image : images) {
                    charge_identity(budget, image.resource_id,
                                    "cross-section tiled image identity index");
                    if (!image_identities.insert(image.resource_id).second) {
                        throw ProjectContainerError(
                            ProjectContainerErrorCode::invalid_tile,
                            "container repeats a tiled image resource identity across sections");
                    }
                    result.container.tiled_images.push_back(std::move(image));
                }
                continue;
            } catch (const UnsupportedTileSection& error) {
                append_opaque_section(result, kind, version, payload, error.what(), budget);
                continue;
            }
        } else if (kind == project_resource_section_kind &&
                   version == project_resource_section_version) {
            try {
                auto resources = decode_resources(payload, limits, total_resources, budget);
                for (ProjectResource& resource : resources) {
                    charge_identity(budget, resource.identifier,
                                    "cross-section project resource identity index");
                    if (!resource_identities.insert(resource.identifier).second) {
                        throw ProjectContainerError(
                            ProjectContainerErrorCode::invalid_resource,
                            "container repeats a project resource identity across sections");
                    }
                    result.container.resources.push_back(std::move(resource));
                }
                continue;
            } catch (const UnsupportedResourceSection& error) {
                append_opaque_section(result, kind, version, payload, error.what(), budget);
                continue;
            }
        } else if (kind == standalone_asset_section_kind &&
                   version == standalone_asset_section_version) {
            auto assets =
                decode_assets(payload, limits, total_assets, total_asset_dependencies, budget);
            for (StandaloneAsset& asset : assets) {
                charge_identity(budget, asset.identifier,
                                "cross-section standalone asset identity index");
                if (!asset_identities.insert(asset.identifier).second) {
                    throw ProjectContainerError(
                        ProjectContainerErrorCode::invalid_asset,
                        "container repeats a standalone asset identity across sections");
                }
                result.container.assets.push_back(std::move(asset));
            }
            continue;
        } else {
            append_opaque_section(result, kind, version, payload,
                                  "container section kind or version is not understood", budget);
            continue;
        }
    }
    if (!body.empty()) {
        throw ProjectContainerError(ProjectContainerErrorCode::malformed_section,
                                    "container body has trailing bytes");
    }
    validate_asset_dependencies(result.container, &resource_identities, &image_identities);
    return result;
}

StoredTiledImage snapshot_tiled_image(std::string resource_id, const image::TiledImage& image) {
    StoredTiledImage stored{.resource_id = std::move(resource_id),
                            .width = image.width(),
                            .height = image.height(),
                            .tile_size = image.tile_size(),
                            .format = image.format(),
                            .clear_pixel = {image.clear_pixel().begin(), image.clear_pixel().end()},
                            .occupied_tiles = {}};
    if (stored.resource_id.empty()) {
        throw ProjectContainerError(ProjectContainerErrorCode::invalid_tile,
                                    "stored tiled image requires a resource identity");
    }
    for (const image::TileCoordinate coordinate : image.allocated_tiles()) {
        const image::TileExtent extent = image.tile_extent(coordinate);
        const image::TileStorageHandle storage = image.pin_tile_storage(coordinate);
        StoredTile tile{.coordinate = coordinate,
                        .extent = extent,
                        .compression = TileCompression::zlib_deflate,
                        .pixels = {}};
        const std::size_t row_bytes = extent.width * image.pixel_bytes();
        tile.pixels.reserve(row_bytes * extent.height);
        for (std::uint32_t row = 0; row < extent.height; ++row) {
            const auto begin = storage->begin() +
                               static_cast<std::ptrdiff_t>(static_cast<std::size_t>(row) *
                                                           image.tile_size() * image.pixel_bytes());
            tile.pixels.insert(tile.pixels.end(), begin,
                               begin + static_cast<std::ptrdiff_t>(row_bytes));
        }
        stored.occupied_tiles.push_back(std::move(tile));
    }
    return stored;
}

image::TiledImage restore_tiled_image(const StoredTiledImage& stored) {
    validate_image(stored);
    image::TiledImage restored(stored.width, stored.height, stored.format, stored.tile_size,
                               stored.clear_pixel);
    const std::size_t pixel_bytes = stored.format.bytes_per_pixel();
    for (const StoredTile& tile : stored.occupied_tiles) {
        for (std::uint32_t y = 0; y < tile.extent.height; ++y) {
            for (std::uint32_t x = 0; x < tile.extent.width; ++x) {
                const std::size_t offset =
                    (static_cast<std::size_t>(y) * tile.extent.width + x) * pixel_bytes;
                restored.write_pixel(tile.coordinate.x * stored.tile_size + x,
                                     tile.coordinate.y * stored.tile_size + y,
                                     std::span(tile.pixels).subspan(offset, pixel_bytes));
            }
        }
    }
    restored.clear_dirty();
    return restored;
}

ProjectResourceResolution resolve_project_resources(
    const ProjectContainer& container,
    std::span<const std::filesystem::path> resource_search_paths) {
    ProjectResourceResolution resolution;
    resolution.resources.reserve(container.resources.size());
    for (const ProjectResource& resource : container.resources) {
        validate_resource(resource);
        ResolvedProjectResource resolved{.identifier = resource.identifier,
                                         .kind = resource.kind,
                                         .relative_path = resource.relative_path,
                                         .status = ProjectResourceStatus::missing,
                                         .bytes = {}};
        if (resource.packed_bytes.has_value()) {
            resolved.status = ProjectResourceStatus::packed;
            resolved.bytes = *resource.packed_bytes;
        } else {
            for (const std::filesystem::path& search_path : resource_search_paths) {
                const std::filesystem::path path = search_path / resource.relative_path;
                std::ifstream stream(path, std::ios::binary | std::ios::ate);
                const std::streampos end = stream.tellg();
                if (stream && end >= 0 &&
                    static_cast<std::uintmax_t>(end) <=
                        static_cast<std::uintmax_t>(std::numeric_limits<std::streamsize>::max())) {
                    resolved.bytes.resize(static_cast<std::size_t>(end));
                    stream.seekg(0);
                    stream.read(reinterpret_cast<char*>(resolved.bytes.data()),
                                static_cast<std::streamsize>(resolved.bytes.size()));
                    if (stream) {
                        resolved.status = ProjectResourceStatus::referenced;
                        break;
                    }
                    resolved.bytes.clear();
                }
            }
        }
        if (resolved.status == ProjectResourceStatus::missing) {
            resolution.missing_identifiers.push_back(resource.identifier);
        }
        resolution.resources.push_back(std::move(resolved));
    }
    return resolution;
}

ProjectResourceResolution resolve_project_resources(
    const ProjectContainer& container, const std::filesystem::path& project_directory) {
    const std::array search_paths{project_directory};
    return resolve_project_resources(container, search_paths);
}

}  // namespace ctex::io
