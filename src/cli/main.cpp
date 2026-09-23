#include <array>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <ctex/doc/smart_material.hpp>
#include <ctex/io/project_container.hpp>
#include <ctex/paint/stroke_preset.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace {

enum class ExitCode : int {
    success = 0,
    invalid_arguments = 2,
    missing_input = 3,
    unsupported_operation = 4,
    missing_resource = 5,
    cancelled = 6,
    over_budget = 7,
    internal_error = 70,
};

struct OptionSpec {
    std::string_view name;
    std::string_view value;
    bool required{};
};

struct CommandSpec {
    std::string_view name;
    std::string_view summary;
    std::span<const OptionSpec> options;
};

constexpr std::array export_options{
    OptionSpec{"--document", "PATH", true}, OptionSpec{"--output", "DIRECTORY", true},
    OptionSpec{"--preset", "NAME", true}, OptionSpec{"--mesh", "PATH", false},
    OptionSpec{"--mesh-policy", "keep|clear|reproject", false}};
constexpr std::array bake_options{OptionSpec{"--document", "PATH", true},
                                  OptionSpec{"--provider", "NAME", true},
                                  OptionSpec{"--output", "PATH", true}};
constexpr std::array apply_options{
    OptionSpec{"--document", "PATH", true}, OptionSpec{"--preset", "PATH", true},
    OptionSpec{"--texture-set", "ID", true}, OptionSpec{"--output", "PATH", true}};
constexpr std::array run_options{OptionSpec{"--document", "PATH", true},
                                 OptionSpec{"--script", "PATH", true},
                                 OptionSpec{"--output", "PATH", true}};
constexpr std::array info_options{OptionSpec{"--document", "PATH", true}};
constexpr std::array validate_options{OptionSpec{"--input", "PATH", true},
                                      OptionSpec{"--kind", "document|material|preset", true}};

constexpr std::array commands{
    CommandSpec{"export", "write texture sets from a document", export_options},
    CommandSpec{"bake-request", "request and bind maps from a bake provider", bake_options},
    CommandSpec{"apply", "apply a smart material or preset", apply_options},
    CommandSpec{"run", "run a Python script against a document", run_options},
    CommandSpec{"info", "report document contents and estimated sizes", info_options},
    CommandSpec{"validate", "validate a document, material, or preset", validate_options},
};

constexpr std::array global_options{
    OptionSpec{"--report", "text|json", false}, OptionSpec{"--executor", "cpu|auto|host", false},
    OptionSpec{"--memory-ceiling", "BYTES", false}, OptionSpec{"--texel-ceiling", "COUNT", false},
    OptionSpec{"--workers", "COUNT", false}};

struct ParsedOption {
    std::string_view name;
    std::string_view value;
};

struct Invocation {
    const CommandSpec* command{};
    std::vector<ParsedOption> options;
    bool quiet{};
};

const CommandSpec* find_command(std::string_view name) {
    for (const CommandSpec& command : commands) {
        if (command.name == name) {
            return &command;
        }
    }
    return nullptr;
}

const OptionSpec* find_option(const CommandSpec& command, std::string_view name) {
    for (const OptionSpec& option : command.options) {
        if (option.name == name) {
            return &option;
        }
    }
    for (const OptionSpec& option : global_options) {
        if (option.name == name) {
            return &option;
        }
    }
    return nullptr;
}

std::optional<std::string_view> option_value(const Invocation& invocation, std::string_view name) {
    for (const ParsedOption& option : invocation.options) {
        if (option.name == name) {
            return option.value;
        }
    }
    return std::nullopt;
}

void print_usage(std::ostream& output) {
    output << "CyberTexel headless command line\n\n"
              "Usage: cybertexel <command> [options]\n\nCommands:\n";
    for (const CommandSpec& command : commands) {
        output << "  " << command.name << "\t" << command.summary << '\n';
    }
    output << "\nGlobal options:\n"
              "  --report text|json       report format (default: text)\n"
              "  --quiet                  suppress non-diagnostic prose\n"
              "  --executor cpu|auto|host executor (default: CTEX_EXECUTOR or cpu)\n"
              "  --memory-ceiling BYTES   maximum working bytes (default: unlimited)\n"
              "  --texel-ceiling COUNT    maximum processed texels (default: unlimited)\n"
              "  --workers COUNT          worker bound (default: hardware concurrency)\n"
              "  --help                   show help\n";
}

void print_command_help(const CommandSpec& command, std::ostream& output) {
    output << "Usage: cybertexel " << command.name << " [options]\n\n"
           << command.summary << ".\n\nOptions:\n";
    for (const OptionSpec& option : command.options) {
        output << "  " << option.name << ' ' << option.value;
        output << (option.required ? " (required)" : " (optional)") << '\n';
    }
    output << "  --report text|json\n  --quiet\n  --executor cpu|auto|host\n"
              "  --memory-ceiling BYTES\n  --texel-ceiling COUNT\n  --workers COUNT\n  --help\n";
}

bool is_positive_integer(std::string_view value) {
    std::uint64_t parsed{};
    const auto result = std::from_chars(value.data(), value.data() + value.size(), parsed);
    return !value.empty() && result.ec == std::errc{} &&
           result.ptr == value.data() + value.size() && parsed > 0;
}

std::optional<std::string> validate_values(const Invocation& invocation) {
    const auto accepted = [&](std::string_view name, std::span<const std::string_view> values) {
        const auto value = option_value(invocation, name);
        if (!value.has_value()) {
            return true;
        }
        for (const std::string_view candidate : values) {
            if (*value == candidate) {
                return true;
            }
        }
        return false;
    };
    constexpr std::array report_values{std::string_view{"text"}, std::string_view{"json"}};
    constexpr std::array executor_values{std::string_view{"cpu"}, std::string_view{"auto"},
                                         std::string_view{"host"}};
    constexpr std::array mesh_policy_values{std::string_view{"keep"}, std::string_view{"clear"},
                                            std::string_view{"reproject"}};
    constexpr std::array kind_values{std::string_view{"document"}, std::string_view{"material"},
                                     std::string_view{"preset"}};
    if (!accepted("--report", report_values)) {
        return "--report accepts: text, json";
    }
    if (!accepted("--executor", executor_values)) {
        return "--executor accepts: cpu, auto, host";
    }
    if (!accepted("--mesh-policy", mesh_policy_values)) {
        return "--mesh-policy accepts: keep, clear, reproject";
    }
    if (!accepted("--kind", kind_values)) {
        return "--kind accepts: document, material, preset";
    }
    for (const std::string_view name : {"--memory-ceiling", "--texel-ceiling", "--workers"}) {
        const auto value = option_value(invocation, name);
        if (value.has_value() && !is_positive_integer(*value)) {
            return std::string(name) + " requires a positive integer";
        }
    }
    return std::nullopt;
}

std::optional<Invocation> parse_invocation(int argc, char** argv, std::string& error) {
    if (argc < 2) {
        error =
            "missing command; accepted commands: export, bake-request, apply, run, info, validate";
        return std::nullopt;
    }
    const CommandSpec* command = find_command(argv[1]);
    if (command == nullptr) {
        error = "unknown command '" + std::string(argv[1]) +
                "'; accepted commands: export, bake-request, apply, run, info, validate";
        return std::nullopt;
    }
    Invocation invocation{.command = command};
    for (int index = 2; index < argc; ++index) {
        const std::string_view argument = argv[index];
        if (argument == "--quiet") {
            invocation.quiet = true;
            continue;
        }
        if (argument == "--help") {
            print_command_help(*command, std::cout);
            std::exit(static_cast<int>(ExitCode::success));
        }
        const OptionSpec* option = find_option(*command, argument);
        if (option == nullptr) {
            error = "unknown option '" + std::string(argument) + "' for command '" +
                    std::string(command->name) + "'";
            return std::nullopt;
        }
        if (index + 1 >= argc) {
            error = std::string(argument) + " requires " + std::string(option->value);
            return std::nullopt;
        }
        for (const ParsedOption& parsed : invocation.options) {
            if (parsed.name == argument) {
                error = "duplicate option '" + std::string(argument) + "'";
                return std::nullopt;
            }
        }
        invocation.options.push_back({argument, argv[++index]});
    }
    for (const OptionSpec& option : command->options) {
        if (option.required && !option_value(invocation, option.name).has_value()) {
            error = "missing required option '" + std::string(option.name) + "' (" +
                    std::string(option.value) + ")";
            return std::nullopt;
        }
    }
    if (const auto invalid = validate_values(invocation); invalid.has_value()) {
        error = *invalid;
        return std::nullopt;
    }
    return invocation;
}

int dispatch(const Invocation& invocation) {
    const auto read_input = [&](std::string_view option) -> std::optional<std::vector<std::byte>> {
        const std::filesystem::path path{*option_value(invocation, option)};
        std::ifstream stream(path, std::ios::binary | std::ios::ate);
        if (!stream) {
            std::cerr << "could not read input '" << path.string() << "'\n";
            return std::nullopt;
        }
        const std::streampos end = stream.tellg();
        if (end < 0) {
            std::cerr << "could not determine input size for '" << path.string() << "'\n";
            return std::nullopt;
        }
        std::vector<std::byte> bytes(static_cast<std::size_t>(end));
        stream.seekg(0);
        if (!bytes.empty() && !stream.read(reinterpret_cast<char*>(bytes.data()),
                                           static_cast<std::streamsize>(bytes.size()))) {
            std::cerr << "could not read complete input '" << path.string() << "'\n";
            return std::nullopt;
        }
        return bytes;
    };
    const auto input_text = [](const std::vector<std::byte>& bytes) {
        return std::string_view(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    };
    const bool json = option_value(invocation, "--report") == "json";
    if (invocation.command->name == "validate") {
        const auto bytes = read_input("--input");
        if (!bytes.has_value()) {
            return static_cast<int>(ExitCode::missing_input);
        }
        const std::string_view kind = *option_value(invocation, "--kind");
        try {
            if (kind == "document") {
                static_cast<void>(ctex::io::read_project_container(*bytes));
            } else if (kind == "material") {
                static_cast<void>(ctex::doc::deserialize_smart_material(input_text(*bytes)));
            } else {
                static_cast<void>(ctex::paint::deserialize_stroke_preset(input_text(*bytes)));
            }
        } catch (const std::exception& error) {
            std::cerr << "invalid " << kind << ": " << error.what() << '\n';
            return static_cast<int>(ExitCode::invalid_arguments);
        }
        if (json) {
            std::cout << "{\"command\":\"validate\",\"executor\":\"cpu\",\"kind\":\"" << kind
                      << "\",\"status\":\"valid\"}\n";
        } else if (!invocation.quiet) {
            std::cout << "valid " << kind << '\n';
        }
        return static_cast<int>(ExitCode::success);
    }
    if (invocation.command->name == "info") {
        const auto bytes = read_input("--document");
        if (!bytes.has_value()) {
            return static_cast<int>(ExitCode::missing_input);
        }
        try {
            const ctex::io::ProjectContainerReadResult result =
                ctex::io::read_project_container(*bytes);
            const auto& container = result.container;
            std::size_t decoded_bytes = 0;
            std::size_t occupied_tiles = 0;
            for (const ctex::io::StoredTiledImage& image : container.tiled_images) {
                decoded_bytes += static_cast<std::size_t>(image.width) * image.height *
                                 image.format.bytes_per_pixel();
                occupied_tiles += image.occupied_tiles.size();
            }
            if (json) {
                std::cout << "{\"assets\":" << container.assets.size()
                          << ",\"command\":\"info\",\"decoded_image_bytes\":" << decoded_bytes
                          << ",\"executor\":\"cpu\",\"file_bytes\":" << bytes->size()
                          << ",\"newer_schema\":" << (result.report.newer_schema ? "true" : "false")
                          << ",\"occupied_tiles\":" << occupied_tiles
                          << ",\"opaque_sections\":" << container.opaque_sections.size()
                          << ",\"resources\":" << container.resources.size() << ",\"schema\":\""
                          << container.schema_version.major << '.' << container.schema_version.minor
                          << '.' << container.schema_version.patch
                          << "\",\"status\":\"ok\",\"tiled_images\":"
                          << container.tiled_images.size()
                          << ",\"unknown_parts\":" << result.report.unknown_parts.size() << "}\n";
            } else if (!invocation.quiet) {
                std::cout << "schema: " << container.schema_version.major << '.'
                          << container.schema_version.minor << '.' << container.schema_version.patch
                          << "\ntiled images: " << container.tiled_images.size()
                          << "\noccupied tiles: " << occupied_tiles
                          << "\nresources: " << container.resources.size()
                          << "\nassets: " << container.assets.size()
                          << "\ndecoded image bytes: " << decoded_bytes << '\n';
            }
            return static_cast<int>(ExitCode::success);
        } catch (const std::exception& error) {
            std::cerr << "invalid document: " << error.what() << '\n';
            return static_cast<int>(ExitCode::invalid_arguments);
        }
    }
    if (json) {
        std::cout << "{\"command\":\"" << invocation.command->name
                  << "\",\"executor\":\"cpu\",\"status\":\"unsupported\"}\n";
    } else if (!invocation.quiet) {
        std::cout << "CyberTexel " << invocation.command->name << "\n";
    }
    std::cerr << "command '" << invocation.command->name
              << "' is not implemented yet (roadmap task 15.2)\n";
    return static_cast<int>(ExitCode::unsupported_operation);
}

}  // namespace

int main(int argc, char** argv) {
    if (argc == 2 && std::string_view(argv[1]) == "--help") {
        print_usage(std::cout);
        return static_cast<int>(ExitCode::success);
    }
    std::string error;
    const std::optional invocation = parse_invocation(argc, argv, error);
    if (!invocation.has_value()) {
        std::cerr << error << '\n';
        return static_cast<int>(ExitCode::invalid_arguments);
    }
    return dispatch(*invocation);
}
