#include <algorithm>
#include <array>
#include <cerrno>
#include <charconv>
#include <chrono>
#include <csignal>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <ctex/doc/smart_material.hpp>
#include <ctex/exec/cpu_reference.hpp>
#include <ctex/exec/executor.hpp>
#include <ctex/io/project_container.hpp>
#include <ctex/io/texture_document.hpp>
#include <ctex/io/texture_export.hpp>
#include <ctex/paint/stroke_preset.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <memory>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#if defined(_WIN32)
#define NOMINMAX
#include <process.h>
#include <windows.h>
#else
#include <sys/wait.h>
#include <unistd.h>
#endif

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

volatile std::sig_atomic_t interrupt_requested = 0;

extern "C" void handle_interrupt(int) { interrupt_requested = 1; }

struct OptionSpec {
    std::string_view name;
    std::string_view value;
    bool required{};
    std::string_view description;
    std::string_view default_value;

    constexpr OptionSpec(std::string_view option_name, std::string_view option_value,
                         bool is_required, std::string_view option_description = {},
                         std::string_view option_default = {})
        : name(option_name),
          value(option_value),
          required(is_required),
          description(option_description),
          default_value(option_default) {}
};

struct CommandSpec {
    std::string_view name;
    std::string_view summary;
    std::span<const OptionSpec> options;
};

constexpr std::array export_options{
    OptionSpec{"--document", "PATH", true, "input project"},
    OptionSpec{"--output", "DIRECTORY", true, "new output directory"},
    OptionSpec{"--preset", "NAME", true, "built-in export preset"},
    OptionSpec{"--mesh", "PATH", false, "replacement mesh (reserved; unsupported)"},
    OptionSpec{"--mesh-policy", "keep|clear|reproject", false,
               "replacement policy (requires --mesh)"}};
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
    OptionSpec{"--report", "text|json", false, "report format", "text"},
    OptionSpec{"--quiet", "", false, "suppress non-diagnostic prose", "false"},
    OptionSpec{"--executor", "cpu|auto|host", false, "executor", "CTEX_EXECUTOR or auto"},
    OptionSpec{"--memory-ceiling", "BYTES", false, "maximum working bytes", "unlimited"},
    OptionSpec{"--texel-ceiling", "COUNT", false, "maximum processed texels", "unlimited"},
    OptionSpec{"--workers", "COUNT", false, "worker bound", "hardware concurrency"},
    OptionSpec{"--help", "", false, "show help", ""}};

struct ParsedOption {
    std::string_view name;
    std::string_view value;
};

struct Invocation {
    const CommandSpec* command{};
    std::vector<ParsedOption> options;
    bool quiet{};
};

struct ExecutionOptions {
    std::uint64_t memory_ceiling{std::numeric_limits<std::uint64_t>::max()};
    std::uint64_t texel_ceiling{std::numeric_limits<std::uint64_t>::max()};
    std::uint64_t workers{std::max(1U, std::thread::hardware_concurrency())};
};

struct SelectedExecutor {
    std::string requested;
    std::string selected;
    std::string source;
    std::string message;
    bool fallback{};
};

class CliError final : public std::runtime_error {
public:
    CliError(ExitCode code, std::string message)
        : std::runtime_error(std::move(message)), code_(code) {}
    [[nodiscard]] ExitCode code() const noexcept { return code_; }

private:
    ExitCode code_;
};

void throw_if_interrupted() {
    if (interrupt_requested != 0) {
        throw CliError(ExitCode::cancelled, "operation was interrupted");
    }
}

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

std::string json_string(std::string_view value) {
    std::string escaped{"\""};
    for (const char character : value) {
        switch (character) {
            case '\"':
                escaped += "\\\"";
                break;
            case '\\':
                escaped += "\\\\";
                break;
            case '\b':
                escaped += "\\b";
                break;
            case '\f':
                escaped += "\\f";
                break;
            case '\n':
                escaped += "\\n";
                break;
            case '\r':
                escaped += "\\r";
                break;
            case '\t':
                escaped += "\\t";
                break;
            default:
                if (static_cast<unsigned char>(character) < 0x20U) {
                    constexpr char hexadecimal[] = "0123456789abcdef";
                    escaped += "\\u00";
                    escaped += hexadecimal[(static_cast<unsigned char>(character) >> 4U) & 0x0fU];
                    escaped += hexadecimal[static_cast<unsigned char>(character) & 0x0fU];
                } else {
                    escaped += character;
                }
        }
    }
    escaped += '\"';
    return escaped;
}

std::uint64_t parse_positive_integer(std::string_view value) {
    std::uint64_t parsed{};
    const auto result = std::from_chars(value.data(), value.data() + value.size(), parsed);
    if (value.empty() || result.ec != std::errc{} || result.ptr != value.data() + value.size() ||
        parsed == 0) {
        throw CliError(ExitCode::invalid_arguments, "expected a positive integer");
    }
    return parsed;
}

ExecutionOptions execution_options(const Invocation& invocation) {
    ExecutionOptions options;
    if (const auto value = option_value(invocation, "--memory-ceiling")) {
        options.memory_ceiling = parse_positive_integer(*value);
    }
    if (const auto value = option_value(invocation, "--texel-ceiling")) {
        options.texel_ceiling = parse_positive_integer(*value);
    }
    if (const auto value = option_value(invocation, "--workers")) {
        options.workers = parse_positive_integer(*value);
    }
    return options;
}

SelectedExecutor select_executor(const Invocation& invocation) {
    ctex::exec::ExecutorRegistry registry;
    registry.add(std::make_shared<ctex::exec::CpuReferenceExecutor>());

    const auto requested_by_flag = option_value(invocation, "--executor");
    const char* environment = std::getenv(ctex::exec::executor_environment_variable.data());
    const std::optional<std::string_view> requested_by_environment =
        environment == nullptr || std::string_view(environment).empty()
            ? std::nullopt
            : std::optional<std::string_view>{environment};
    const std::string_view requested =
        requested_by_flag.value_or(requested_by_environment.value_or(std::string_view{"auto"}));
    const std::string source = requested_by_flag.has_value()          ? "flag"
                               : requested_by_environment.has_value() ? "environment"
                                                                      : "default";

    if (requested == "cpu" || requested == "auto") {
        const ctex::exec::ExecutorSelection selected =
            requested == "cpu" ? registry.select("cpu") : registry.select_automatic();
        return {.requested = std::string(requested),
                .selected = selected.executor->descriptor().identifier,
                .source = source,
                .message = selected.message,
                .fallback = false};
    }

    const ctex::exec::ExecutorSelection fallback = registry.select_automatic();
    const std::string message =
        requested == "host"
            ? "host execution is not attached; fell back to CPU reference"
            : "unknown executor '" + std::string(requested) + "'; fell back to CPU reference";
    return {.requested = std::string(requested),
            .selected = fallback.executor->descriptor().identifier,
            .source = source,
            .message = message,
            .fallback = true};
}

bool wants_json_report(int argc, char** argv) {
    for (int index = 1; index + 1 < argc; ++index) {
        if (std::string_view(argv[index]) == "--report" &&
            std::string_view(argv[index + 1]) == "json") {
            return true;
        }
    }
    return false;
}

std::string executor_report_fields(const SelectedExecutor& executor) {
    return "\"executor\":" + json_string(executor.selected) +
           ",\"executor_requested\":" + json_string(executor.requested) +
           ",\"executor_source\":" + json_string(executor.source) +
           ",\"fallback\":" + (executor.fallback ? "true" : "false") +
           ",\"executor_message\":" + json_string(executor.message);
}

void emit_parse_failure_json(int argc, char** argv, std::string_view diagnostic) {
    const std::string_view command = argc > 1 ? std::string_view(argv[1]) : std::string_view{};
    std::cout << "{\"command\":" << json_string(command)
              << ",\"diagnostic\":" << json_string(diagnostic)
              << ",\"exit_code\":" << static_cast<int>(ExitCode::invalid_arguments)
              << ",\"status\":\"invalid_arguments\"}\n";
}

void print_option(const OptionSpec& option, std::ostream& output, bool command_option) {
    output << "  " << option.name;
    if (!option.value.empty()) {
        output << ' ' << option.value;
    }
    if (!option.description.empty()) {
        output << "  " << option.description;
    }
    if (command_option && option.required) {
        output << " (required)";
    } else if (command_option) {
        output << " (optional; default: not set)";
    } else if (!option.default_value.empty()) {
        output << " (default: " << option.default_value << ')';
    }
    output << '\n';
}

void print_global_options(std::ostream& output) {
    for (const OptionSpec& option : global_options) {
        print_option(option, output, false);
    }
}

void print_usage(std::ostream& output) {
    output << "CyberTexel headless command line\n\n"
              "Usage: cybertexel <command> [options]\n\nCommands:\n";
    for (const CommandSpec& command : commands) {
        output << "  " << command.name << "\t" << command.summary << '\n';
    }
    output << "\nGlobal options:\n";
    print_global_options(output);
}

void print_command_help(const CommandSpec& command, std::ostream& output) {
    output << "Usage: cybertexel " << command.name << " [options]\n\n"
           << command.summary << ".\n\nOptions:\n";
    for (const OptionSpec& option : command.options) {
        print_option(option, output, true);
    }
    print_global_options(output);
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
    Invocation invocation{.command = command, .options = {}, .quiet = false};
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

std::vector<std::byte> read_input(const std::filesystem::path& path,
                                  const ExecutionOptions& options) {
    throw_if_interrupted();
    std::ifstream stream(path, std::ios::binary | std::ios::ate);
    if (!stream) {
        throw CliError(ExitCode::missing_input, "could not read input '" + path.string() + "'");
    }
    const std::streampos end = stream.tellg();
    if (end < 0) {
        throw CliError(ExitCode::missing_input,
                       "could not determine input size for '" + path.string() + "'");
    }
    const auto input_bytes = static_cast<std::uint64_t>(end);
    if (input_bytes > options.memory_ceiling) {
        throw CliError(ExitCode::over_budget,
                       "input '" + path.string() + "' requires " + std::to_string(input_bytes) +
                           " bytes; memory ceiling is " + std::to_string(options.memory_ceiling));
    }
    if (input_bytes > std::numeric_limits<std::size_t>::max()) {
        throw CliError(ExitCode::over_budget, "input size exceeds this platform's address space");
    }
    std::vector<std::byte> bytes(static_cast<std::size_t>(input_bytes));
    stream.seekg(0);
    if (!bytes.empty() && !stream.read(reinterpret_cast<char*>(bytes.data()),
                                       static_cast<std::streamsize>(bytes.size()))) {
        throw CliError(ExitCode::missing_input,
                       "could not read complete input '" + path.string() + "'");
    }
    throw_if_interrupted();
    return bytes;
}

std::string_view input_text(const std::vector<std::byte>& bytes) {
    return {reinterpret_cast<const char*>(bytes.data()), bytes.size()};
}

ctex::io::ProjectContainerReadLimits project_limits(const ExecutionOptions& options) {
    ctex::io::ProjectContainerReadLimits limits;
    const std::uint64_t maximum_size = std::numeric_limits<std::size_t>::max();
    const auto ceiling = static_cast<std::size_t>(std::min(options.memory_ceiling, maximum_size));
    limits.maximum_input_bytes = ceiling;
    limits.maximum_total_allocation_bytes = ceiling;
    return limits;
}

ctex::io::TextureDocumentReadLimits texture_document_limits(const ExecutionOptions& options) {
    ctex::io::TextureDocumentReadLimits limits;
    const auto ceiling = static_cast<std::size_t>(
        std::min(options.memory_ceiling,
                 static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())));
    limits.maximum_payload_bytes = ceiling;
    limits.maximum_embedded_blob_bytes = ceiling;
    return limits;
}

std::uint64_t checked_product(std::uint64_t left, std::uint64_t right,
                              std::string_view description) {
    if (right != 0 && left > std::numeric_limits<std::uint64_t>::max() / right) {
        throw CliError(ExitCode::over_budget,
                       std::string(description) + " exceeds the supported counter range");
    }
    return left * right;
}

struct ContainerMetrics {
    std::uint64_t texels{};
    std::uint64_t decoded_bytes{};
    std::uint64_t occupied_tiles{};
    std::uint64_t documents{};
    std::uint64_t texture_sets{};
    std::uint64_t layer_entries{};
    std::uint64_t atlases{};
    std::uint64_t editable_entries{};
    std::uint64_t preset_applications{};
};

ContainerMetrics measure_container(const ctex::io::ProjectContainer& container,
                                   const ExecutionOptions& options) {
    ContainerMetrics metrics;
    for (const ctex::io::StoredTiledImage& image : container.tiled_images) {
        const std::uint64_t image_texels =
            checked_product(image.width, image.height, "texel count");
        if (image_texels >
            options.texel_ceiling - std::min(options.texel_ceiling, metrics.texels)) {
            throw CliError(ExitCode::over_budget, "document exceeds texel ceiling " +
                                                      std::to_string(options.texel_ceiling));
        }
        metrics.texels += image_texels;
        const std::uint64_t image_bytes =
            checked_product(image_texels, image.format.bytes_per_pixel(), "decoded image size");
        if (image_bytes > std::numeric_limits<std::uint64_t>::max() - metrics.decoded_bytes) {
            throw CliError(ExitCode::over_budget, "decoded image size exceeds counter range");
        }
        metrics.decoded_bytes += image_bytes;
        metrics.occupied_tiles += image.occupied_tiles.size();
    }
    return metrics;
}

void add_document_metrics(ContainerMetrics& metrics,
                          std::span<const ctex::io::TextureDocumentAssetInfo> documents) {
    const auto add = [](std::uint64_t& total, std::size_t value) {
        if (value > std::numeric_limits<std::uint64_t>::max() - total) {
            throw CliError(ExitCode::over_budget, "document inventory count exceeds counter range");
        }
        total += value;
    };
    metrics.documents = documents.size();
    for (const ctex::io::TextureDocumentAssetInfo& document : documents) {
        add(metrics.texture_sets, document.texture_set_count);
        add(metrics.layer_entries, document.layer_entry_count);
        add(metrics.atlases, document.atlas_count);
        add(metrics.editable_entries, document.editable_entry_count);
        add(metrics.preset_applications, document.preset_application_count);
    }
}

using SteadyTime = std::chrono::steady_clock::time_point;

double elapsed_milliseconds(SteadyTime started) {
    return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - started)
        .count();
}

std::string standard_report_fields(const Invocation& invocation, const SelectedExecutor& executor,
                                   const ExecutionOptions& options, ExitCode exit_code,
                                   std::string_view status, double elapsed_ms) {
    return "\"command\":" + json_string(invocation.command->name) +
           ",\"status\":" + json_string(status) +
           ",\"exit_code\":" + std::to_string(static_cast<int>(exit_code)) + ',' +
           executor_report_fields(executor) +
           ",\"limits\":{\"memory_bytes\":" + std::to_string(options.memory_ceiling) +
           ",\"texels\":" + std::to_string(options.texel_ceiling) +
           ",\"workers\":" + std::to_string(options.workers) + '}' +
           ",\"clamped_parameters\":[],\"timings_ms\":{\"total\":" + std::to_string(elapsed_ms) +
           '}';
}

void emit_error_report(const Invocation& invocation, const SelectedExecutor& executor,
                       const ExecutionOptions& options, const CliError& error, double elapsed_ms) {
    if (option_value(invocation, "--report") == "json") {
        std::cout << '{'
                  << standard_report_fields(invocation, executor, options, error.code(), "error",
                                            elapsed_ms)
                  << ",\"outputs\":[],\"diagnostic\":" << json_string(error.what()) << "}\n";
    }
}

int validate_command(const Invocation& invocation, const SelectedExecutor& executor,
                     const ExecutionOptions& options, SteadyTime started) {
    const std::filesystem::path path{*option_value(invocation, "--input")};
    const std::vector<std::byte> bytes = read_input(path, options);
    const std::string_view kind = *option_value(invocation, "--kind");
    try {
        if (kind == "document") {
            const ctex::io::ProjectContainerReadResult project =
                ctex::io::read_project_container(bytes, project_limits(options));
            static_cast<void>(ctex::io::list_texture_documents(project.container,
                                                               texture_document_limits(options)));
        } else if (kind == "material") {
            static_cast<void>(ctex::doc::deserialize_smart_material(input_text(bytes)));
        } else {
            static_cast<void>(ctex::paint::deserialize_stroke_preset(input_text(bytes)));
        }
    } catch (const ctex::io::ProjectContainerError& error) {
        const ExitCode code = error.code() == ctex::io::ProjectContainerErrorCode::over_limit
                                  ? ExitCode::over_budget
                                  : ExitCode::invalid_arguments;
        throw CliError(code, "invalid " + std::string(kind) + ": " + error.what());
    } catch (const std::exception& error) {
        throw CliError(ExitCode::invalid_arguments,
                       "invalid " + std::string(kind) + ": " + error.what());
    }
    throw_if_interrupted();

    if (option_value(invocation, "--report") == "json") {
        std::cout << '{'
                  << standard_report_fields(invocation, executor, options, ExitCode::success,
                                            "valid", elapsed_milliseconds(started))
                  << ",\"outputs\":[],\"inputs\":[{\"kind\":" << json_string(kind)
                  << ",\"path\":" << json_string(path.string())
                  << "}],\"operations\":[\"read\",\"validate\"],\"kind\":" << json_string(kind)
                  << "}\n";
    } else if (!invocation.quiet) {
        std::cout << "valid " << kind << "\nexecutor: " << executor.selected;
        if (executor.fallback) {
            std::cout << " (fallback from " << executor.requested << ')';
        }
        std::cout << '\n';
    }
    return static_cast<int>(ExitCode::success);
}

int info_command(const Invocation& invocation, const SelectedExecutor& executor,
                 const ExecutionOptions& options, SteadyTime started) {
    const std::filesystem::path path{*option_value(invocation, "--document")};
    const std::vector<std::byte> bytes = read_input(path, options);
    ctex::io::ProjectContainerReadResult result;
    std::vector<ctex::io::TextureDocumentAssetInfo> documents;
    try {
        result = ctex::io::read_project_container(bytes, project_limits(options));
        documents =
            ctex::io::list_texture_documents(result.container, texture_document_limits(options));
    } catch (const ctex::io::ProjectContainerError& error) {
        const ExitCode code = error.code() == ctex::io::ProjectContainerErrorCode::over_limit
                                  ? ExitCode::over_budget
                                  : ExitCode::invalid_arguments;
        throw CliError(code, "invalid document: " + std::string(error.what()));
    } catch (const ctex::io::TextureDocumentIoError& error) {
        throw CliError(ExitCode::invalid_arguments,
                       "invalid document state: " + std::string(error.what()));
    }
    const auto& container = result.container;
    ContainerMetrics metrics = measure_container(container, options);
    add_document_metrics(metrics, documents);
    throw_if_interrupted();
    const std::string schema = std::to_string(container.schema_version.major) + '.' +
                               std::to_string(container.schema_version.minor) + '.' +
                               std::to_string(container.schema_version.patch);
    if (option_value(invocation, "--report") == "json") {
        std::cout << '{'
                  << standard_report_fields(invocation, executor, options, ExitCode::success, "ok",
                                            elapsed_milliseconds(started))
                  << ",\"outputs\":[],\"inputs\":[{\"kind\":\"document\",\"path\":"
                  << json_string(path.string()) << "}],\"operations\":[\"read\",\"inspect\"]"
                  << ",\"assets\":" << container.assets.size()
                  << ",\"decoded_image_bytes\":" << metrics.decoded_bytes
                  << ",\"documents\":" << metrics.documents
                  << ",\"texture_sets\":" << metrics.texture_sets
                  << ",\"layer_entries\":" << metrics.layer_entries
                  << ",\"atlases\":" << metrics.atlases
                  << ",\"editable_entries\":" << metrics.editable_entries
                  << ",\"preset_applications\":" << metrics.preset_applications
                  << ",\"file_bytes\":" << bytes.size()
                  << ",\"newer_schema\":" << (result.report.newer_schema ? "true" : "false")
                  << ",\"occupied_tiles\":" << metrics.occupied_tiles
                  << ",\"opaque_sections\":" << container.opaque_sections.size()
                  << ",\"resources\":" << container.resources.size()
                  << ",\"schema\":" << json_string(schema) << ",\"texels\":" << metrics.texels
                  << ",\"tiled_images\":" << container.tiled_images.size()
                  << ",\"unknown_parts\":" << result.report.unknown_parts.size() << "}\n";
    } else if (!invocation.quiet) {
        std::cout << "schema: " << schema << "\ntiled images: " << container.tiled_images.size()
                  << "\noccupied tiles: " << metrics.occupied_tiles
                  << "\nresources: " << container.resources.size()
                  << "\nassets: " << container.assets.size() << "\ndocuments: " << metrics.documents
                  << "\ntexture sets: " << metrics.texture_sets
                  << "\nlayer entries: " << metrics.layer_entries
                  << "\natlases: " << metrics.atlases
                  << "\neditable entries: " << metrics.editable_entries
                  << "\npreset applications: " << metrics.preset_applications
                  << "\ndecoded image bytes: " << metrics.decoded_bytes
                  << "\nexecutor: " << executor.selected;
        if (executor.fallback) {
            std::cout << " (fallback from " << executor.requested << ')';
        }
        std::cout << '\n';
    }
    return static_cast<int>(ExitCode::success);
}

std::string only_texture_document_identity(const ctex::io::ProjectContainer& project) {
    std::string identity;
    for (const ctex::io::StandaloneAsset& asset : project.assets) {
        if (asset.kind != ctex::io::texture_document_asset_kind) continue;
        if (!identity.empty()) {
            throw CliError(ExitCode::invalid_arguments,
                           "project contains multiple texture documents; selection is ambiguous");
        }
        identity = asset.identifier;
    }
    if (identity.empty()) {
        throw CliError(ExitCode::invalid_arguments,
                       "project does not contain a live texture document");
    }
    return identity;
}

void require_preset_resources(const ctex::doc::SmartMaterialPreset& preset,
                              const ctex::io::ProjectContainer& project,
                              const std::filesystem::path& project_path) {
    for (const ctex::doc::SmartMaterialResourceReference& required : preset.resource_references) {
        const auto found =
            std::ranges::find_if(project.resources, [&](const ctex::io::ProjectResource& resource) {
                return resource.identifier == required.identifier;
            });
        if (found == project.resources.end()) {
            throw CliError(
                ExitCode::missing_resource,
                "smart material requires missing resource '" + required.identifier + "'");
        }
        if (found->packed_bytes.has_value()) continue;
        const std::filesystem::path resolved = project_path.parent_path() / found->relative_path;
        std::ifstream stream(resolved, std::ios::binary);
        if (!stream) {
            throw CliError(ExitCode::missing_resource,
                           "smart material resource '" + required.identifier +
                               "' is unreadable at '" + resolved.string() + "'");
        }
    }
}

std::string next_application_identity(const ctex::doc::TextureSet& texture_set,
                                      std::string_view preset_identity) {
    const auto available = [&](std::string_view candidate) {
        return std::ranges::none_of(texture_set.preset_applications(),
                                    [&](const ctex::doc::AppliedPresetApplication& application) {
                                        return application.identifier == candidate;
                                    });
    };
    if (available(preset_identity)) return std::string(preset_identity);
    for (std::uint64_t suffix = 2; suffix != std::numeric_limits<std::uint64_t>::max(); ++suffix) {
        std::string candidate = std::string(preset_identity) + '-' + std::to_string(suffix);
        if (available(candidate)) return candidate;
    }
    throw CliError(ExitCode::over_budget, "preset application identity space is exhausted");
}

void require_document_memory_budget(const ctex::doc::TextureDocument& document,
                                    std::uint64_t input_bytes, const ExecutionOptions& options) {
    const std::uint64_t resident = document.memory_report().total_resident_bytes;
    const bool overflow = input_bytes > std::numeric_limits<std::uint64_t>::max() - resident;
    const std::uint64_t required =
        overflow ? std::numeric_limits<std::uint64_t>::max() : resident + input_bytes;
    if (required > options.memory_ceiling) {
        throw CliError(ExitCode::over_budget, "apply requires " + std::to_string(required) +
                                                  " resident and input bytes; memory ceiling is " +
                                                  std::to_string(options.memory_ceiling));
    }
}

int apply_command(const Invocation& invocation, const SelectedExecutor& executor,
                  const ExecutionOptions& options, SteadyTime started) {
    const std::filesystem::path document_path{*option_value(invocation, "--document")};
    const std::filesystem::path preset_path{*option_value(invocation, "--preset")};
    const std::filesystem::path output_path{*option_value(invocation, "--output")};
    const std::string texture_set_id{*option_value(invocation, "--texture-set")};
    const std::vector<std::byte> project_bytes = read_input(document_path, options);
    const std::vector<std::byte> preset_bytes = read_input(preset_path, options);

    try {
        ctex::io::ProjectContainer project =
            ctex::io::read_project_container(project_bytes, project_limits(options)).container;
        static_cast<void>(measure_container(project, options));
        const std::string document_identity = only_texture_document_identity(project);
        ctex::doc::TextureDocument document = ctex::io::unpack_texture_document(
            project, document_identity, texture_document_limits(options));
        const std::uint64_t input_bytes =
            project_bytes.size() > std::numeric_limits<std::uint64_t>::max() - preset_bytes.size()
                ? std::numeric_limits<std::uint64_t>::max()
                : project_bytes.size() + preset_bytes.size();
        require_document_memory_budget(document, input_bytes, options);
        if (!document.contains_texture_set(texture_set_id)) {
            throw CliError(ExitCode::invalid_arguments,
                           "texture set does not exist: '" + texture_set_id + "'");
        }
        const ctex::doc::SmartMaterialPreset preset =
            ctex::doc::deserialize_smart_material(input_text(preset_bytes));
        require_preset_resources(preset, project, document_path);
        ctex::doc::TextureSet& texture_set = document.texture_set(texture_set_id);
        const std::string application_identity =
            next_application_identity(texture_set, preset.identifier);
        const ctex::doc::PresetApplicationReport applied =
            texture_set.apply_smart_material(preset, application_identity);
        ctex::io::upsert_texture_document(project, document_identity, document);
        throw_if_interrupted();
        ctex::io::save_project_container_atomic(output_path, project);
        const std::uintmax_t output_bytes = std::filesystem::file_size(output_path);

        if (option_value(invocation, "--report") == "json") {
            std::cout << '{'
                      << standard_report_fields(invocation, executor, options, ExitCode::success,
                                                "ok", elapsed_milliseconds(started))
                      << ",\"outputs\":[{\"kind\":\"project\",\"path\":"
                      << json_string(output_path.string()) << ",\"bytes\":" << output_bytes
                      << "}],\"inputs\":[{\"kind\":\"document\",\"path\":"
                      << json_string(document_path.string())
                      << "},{\"kind\":\"smart-material\",\"path\":"
                      << json_string(preset_path.string())
                      << "}],\"operations\":[\"read\",\"open\",\"apply\",\"save\"]"
                      << ",\"document_asset\":" << json_string(document_identity)
                      << ",\"texture_set\":" << json_string(texture_set_id)
                      << ",\"application\":" << json_string(applied.application_identifier)
                      << ",\"entries_added\":" << applied.entry_identifiers.size() << "}\n";
        } else if (!invocation.quiet) {
            std::cout << "applied " << preset.identifier << " to " << texture_set_id
                      << "\nwrote: " << output_path.string() << "\nexecutor: " << executor.selected
                      << '\n';
        }
        return static_cast<int>(ExitCode::success);
    } catch (const CliError&) {
        throw;
    } catch (const ctex::io::ProjectContainerError& error) {
        const ExitCode code =
            error.code() == ctex::io::ProjectContainerErrorCode::over_limit ? ExitCode::over_budget
            : error.code() == ctex::io::ProjectContainerErrorCode::filesystem_failure ||
                    error.code() == ctex::io::ProjectContainerErrorCode::compression_failed
                ? ExitCode::internal_error
                : ExitCode::invalid_arguments;
        throw CliError(code, "could not apply material: " + std::string(error.what()));
    } catch (const ctex::io::TextureDocumentIoError& error) {
        throw CliError(ExitCode::invalid_arguments,
                       "invalid texture document: " + std::string(error.what()));
    } catch (const ctex::doc::SmartMaterialError& error) {
        throw CliError(ExitCode::invalid_arguments,
                       "invalid smart material: " + std::string(error.what()));
    } catch (const std::invalid_argument& error) {
        throw CliError(ExitCode::invalid_arguments,
                       "could not apply material: " + std::string(error.what()));
    }
}

const ctex::io::ExportPreset& require_export_preset(std::string_view identity) {
    for (const ctex::io::ExportPreset& preset : ctex::io::built_in_export_presets()) {
        if (preset.identifier == identity) return preset;
    }
    std::string message = "unknown export preset '" + std::string(identity) + "'; available:";
    for (const ctex::io::ExportPreset& preset : ctex::io::built_in_export_presets()) {
        message += ' ' + preset.identifier;
    }
    throw CliError(ExitCode::invalid_arguments, std::move(message));
}

std::uint64_t checked_sum(std::uint64_t left, std::uint64_t right, std::string_view description) {
    if (right > std::numeric_limits<std::uint64_t>::max() - left) {
        throw CliError(ExitCode::over_budget,
                       std::string(description) + " exceeds the supported counter range");
    }
    return left + right;
}

void require_export_budget(const ctex::doc::TextureDocument& document,
                           const ctex::io::TextureExportReport& plan, std::uint64_t input_bytes,
                           const ExecutionOptions& options) {
    std::uint64_t total_texels{};
    std::uint64_t estimated_outputs{};
    std::uint64_t maximum_working{};
    for (const ctex::io::TextureExportReportEntry& output : plan.outputs) {
        const std::uint64_t texels = checked_product(output.width, output.height, "export texels");
        total_texels = checked_sum(total_texels, texels, "export texels");
        estimated_outputs =
            checked_sum(estimated_outputs, output.estimated_size_bytes, "export output size");
        maximum_working =
            std::max(maximum_working, checked_product(texels, 128, "export working memory"));
    }
    if (total_texels > options.texel_ceiling) {
        throw CliError(ExitCode::over_budget, "export requires " + std::to_string(total_texels) +
                                                  " texels; texel ceiling is " +
                                                  std::to_string(options.texel_ceiling));
    }
    std::uint64_t required =
        checked_sum(input_bytes, document.memory_report().total_resident_bytes, "export memory");
    required = checked_sum(required, estimated_outputs, "export memory");
    required = checked_sum(required, maximum_working, "export memory");
    if (required > options.memory_ceiling) {
        throw CliError(ExitCode::over_budget,
                       "export requires an estimated " + std::to_string(required) +
                           " bytes; memory ceiling is " + std::to_string(options.memory_ceiling));
    }
}

void validate_export_destination(const std::filesystem::path& destination) {
    if (destination.empty() || destination.filename().empty()) {
        throw CliError(ExitCode::invalid_arguments, "export output must name a new directory");
    }
    std::error_code error;
    const std::filesystem::file_status status = std::filesystem::symlink_status(destination, error);
    if (error && error != std::errc::no_such_file_or_directory) {
        throw CliError(ExitCode::internal_error,
                       "could not inspect export output: " + error.message());
    }
    if (status.type() != std::filesystem::file_type::not_found) {
        throw CliError(ExitCode::invalid_arguments,
                       "export output already exists: '" + destination.string() + "'");
    }
    std::filesystem::path parent = destination.parent_path();
    if (parent.empty()) parent = ".";
    error.clear();
    if (!std::filesystem::is_directory(parent, error) || error) {
        throw CliError(
            ExitCode::invalid_arguments,
            "export output parent is not a readable directory: '" + parent.string() + "'");
    }
}

class StagedOutputDirectory {
public:
    explicit StagedOutputDirectory(std::filesystem::path destination)
        : destination_(std::move(destination)) {
        validate_export_destination(destination_);
        std::error_code error;
        std::filesystem::path parent = destination_.parent_path();
        if (parent.empty()) parent = ".";
        const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
        for (std::uint32_t attempt = 0; attempt < 1024; ++attempt) {
            staging_ = parent / ("." + destination_.filename().string() + ".ctex-stage-" +
                                 std::to_string(nonce) + '-' + std::to_string(attempt));
            error.clear();
            if (std::filesystem::create_directory(staging_, error)) return;
            if (error) {
                throw CliError(ExitCode::internal_error,
                               "could not create staged export directory: " + error.message());
            }
        }
        throw CliError(ExitCode::internal_error,
                       "could not reserve a unique staged export directory");
    }

    StagedOutputDirectory(const StagedOutputDirectory&) = delete;
    StagedOutputDirectory& operator=(const StagedOutputDirectory&) = delete;

    ~StagedOutputDirectory() {
        if (!published_) {
            std::error_code ignored;
            std::filesystem::remove_all(staging_, ignored);
        }
    }

    void write(const ctex::io::InMemoryTextureExport& output) const {
        const std::filesystem::path relative{output.relative_path};
        if (relative.empty() || relative.is_absolute() ||
            std::ranges::any_of(relative, [](const auto& part) { return part == ".."; })) {
            throw CliError(ExitCode::internal_error,
                           "export planner produced an unsafe relative path");
        }
        const std::filesystem::path path = staging_ / relative;
        std::error_code error;
        std::filesystem::create_directories(path.parent_path(), error);
        if (error) {
            throw CliError(ExitCode::internal_error,
                           "could not create export subdirectory: " + error.message());
        }
        if (output.bytes.size() >
            static_cast<std::size_t>(std::numeric_limits<std::streamsize>::max())) {
            throw CliError(ExitCode::over_budget,
                           "encoded export exceeds this platform's file-stream limit");
        }
        std::ofstream stream(path, std::ios::binary | std::ios::trunc);
        stream.write(reinterpret_cast<const char*>(output.bytes.data()),
                     static_cast<std::streamsize>(output.bytes.size()));
        if (!stream) {
            throw CliError(ExitCode::internal_error,
                           "could not write staged export '" + output.relative_path + "'");
        }
    }

    void publish() {
        std::error_code error;
        std::filesystem::rename(staging_, destination_, error);
        if (error) {
            throw CliError(ExitCode::internal_error,
                           "could not publish export directory: " + error.message());
        }
        published_ = true;
    }

private:
    std::filesystem::path destination_;
    std::filesystem::path staging_;
    bool published_{};
};

void validate_project_output(const std::filesystem::path& destination) {
    if (destination.empty() || destination.filename().empty()) {
        throw CliError(ExitCode::invalid_arguments, "run output must name a project file");
    }
    std::error_code error;
    const std::filesystem::file_status status = std::filesystem::symlink_status(destination, error);
    if (error && error != std::errc::no_such_file_or_directory) {
        throw CliError(ExitCode::internal_error,
                       "could not inspect run output: " + error.message());
    }
    if (status.type() == std::filesystem::file_type::directory) {
        throw CliError(ExitCode::invalid_arguments,
                       "run output is a directory: '" + destination.string() + "'");
    }
    std::filesystem::path parent = destination.parent_path();
    if (parent.empty()) parent = ".";
    error.clear();
    if (!std::filesystem::is_directory(parent, error) || error) {
        throw CliError(ExitCode::invalid_arguments,
                       "run output parent is not a directory: '" + parent.string() + "'");
    }
}

class StagedProjectOutput {
public:
    explicit StagedProjectOutput(const std::filesystem::path& destination) {
        validate_project_output(destination);
        std::filesystem::path parent = destination.parent_path();
        if (parent.empty()) parent = ".";
        const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
        std::error_code error;
        for (std::uint32_t attempt = 0; attempt < 1024; ++attempt) {
            directory_ = parent / ("." + destination.filename().string() + ".ctex-run-" +
                                   std::to_string(nonce) + '-' + std::to_string(attempt));
            error.clear();
            if (std::filesystem::create_directory(directory_, error)) {
                path_ = directory_ / "result.ctex";
                return;
            }
            if (error) {
                throw CliError(ExitCode::internal_error,
                               "could not create run staging directory: " + error.message());
            }
        }
        throw CliError(ExitCode::internal_error,
                       "could not reserve a unique run staging directory");
    }

    StagedProjectOutput(const StagedProjectOutput&) = delete;
    StagedProjectOutput& operator=(const StagedProjectOutput&) = delete;

    ~StagedProjectOutput() {
        std::error_code ignored;
        std::filesystem::remove_all(directory_, ignored);
    }

    [[nodiscard]] const std::filesystem::path& path() const noexcept { return path_; }

private:
    std::filesystem::path directory_;
    std::filesystem::path path_;
};

std::string python_executable() {
    const char* configured = std::getenv("CTEX_PYTHON");
    if (configured != nullptr && !std::string_view(configured).empty()) return configured;
#if defined(_WIN32)
    return "python";
#else
    return "python3";
#endif
}

#if defined(_WIN32)
int spawn_python_process(std::vector<std::string>& arguments) {
    std::vector<const char*> raw_arguments;
    raw_arguments.reserve(arguments.size() + 1);
    for (const std::string& argument : arguments) raw_arguments.push_back(argument.c_str());
    raw_arguments.push_back(nullptr);
    const intptr_t result = _spawnvp(_P_NOWAIT, arguments.front().c_str(), raw_arguments.data());
    if (result == -1) return 127;
    const HANDLE process = reinterpret_cast<HANDLE>(result);
    for (;;) {
        const DWORD wait_result = WaitForSingleObject(process, 25);
        if (wait_result == WAIT_OBJECT_0) {
            DWORD exit_code{};
            const BOOL queried = GetExitCodeProcess(process, &exit_code);
            CloseHandle(process);
            if (queried == 0) {
                throw CliError(ExitCode::internal_error, "could not query Python process outcome");
            }
            return static_cast<int>(exit_code);
        }
        if (wait_result == WAIT_FAILED) {
            CloseHandle(process);
            throw CliError(ExitCode::internal_error, "could not wait for Python process");
        }
        if (interrupt_requested != 0) {
            static_cast<void>(TerminateProcess(process, 130));
            static_cast<void>(WaitForSingleObject(process, INFINITE));
            CloseHandle(process);
            return 130;
        }
    }
}
#else
int interrupted_python_process(pid_t child, int& status) {
    static_cast<void>(kill(child, SIGINT));
    while (waitpid(child, &status, 0) < 0 && errno == EINTR) {
    }
    return 130;
}

int wait_for_python_process(pid_t child) {
    int status{};
    for (;;) {
        const pid_t waited = waitpid(child, &status, WNOHANG);
        if (waited == child) break;
        if (waited < 0 && errno != EINTR) {
            throw CliError(ExitCode::internal_error, "could not wait for Python process");
        }
        if (interrupt_requested != 0) return interrupted_python_process(child, status);
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
    }
    if (WIFEXITED(status)) return WEXITSTATUS(status);
    if (WIFSIGNALED(status) && (WTERMSIG(status) == SIGINT || WTERMSIG(status) == SIGTERM)) {
        return 130;
    }
    return 1;
}

int spawn_python_process(std::vector<std::string>& arguments) {
    std::vector<char*> raw_arguments;
    raw_arguments.reserve(arguments.size() + 1);
    for (std::string& argument : arguments) raw_arguments.push_back(argument.data());
    raw_arguments.push_back(nullptr);
    const pid_t child = fork();
    if (child < 0) {
        throw CliError(ExitCode::internal_error, "could not create Python process");
    }
    if (child == 0) {
        execvp(arguments.front().c_str(), raw_arguments.data());
        _exit(127);
    }
    return wait_for_python_process(child);
}
#endif

int spawn_python(std::vector<std::string> arguments) {
    throw_if_interrupted();
    return spawn_python_process(arguments);
}

void require_combined_input_budget(std::uint64_t document_bytes, std::uint64_t script_bytes,
                                   const ExecutionOptions& options) {
    const std::uint64_t required = checked_sum(document_bytes, script_bytes, "run input size");
    if (required > options.memory_ceiling) {
        throw CliError(ExitCode::over_budget, "run inputs require " + std::to_string(required) +
                                                  " bytes; memory ceiling is " +
                                                  std::to_string(options.memory_ceiling));
    }
}

int run_command(const Invocation& invocation, const SelectedExecutor& executor,
                const ExecutionOptions& options, SteadyTime started) {
    const std::filesystem::path document_path{*option_value(invocation, "--document")};
    const std::filesystem::path script_path{*option_value(invocation, "--script")};
    const std::filesystem::path output_path{*option_value(invocation, "--output")};
    validate_project_output(output_path);
    const std::vector<std::byte> project_bytes = read_input(document_path, options);
    const std::vector<std::byte> script_bytes = read_input(script_path, options);
    require_combined_input_budget(project_bytes.size(), script_bytes.size(), options);

    try {
        const ctex::io::ProjectContainer input =
            ctex::io::read_project_container(project_bytes, project_limits(options)).container;
        static_cast<void>(measure_container(input, options));
        const std::string document_identity = only_texture_document_identity(input);
        StagedProjectOutput staged(output_path);
        const std::string interpreter = python_executable();
        const int status =
            spawn_python({interpreter, "-m", "cybertexel._script_runner", document_path.string(),
                          script_path.string(), staged.path().string()});
        if (status == 127) {
            throw CliError(ExitCode::missing_input,
                           "could not launch Python interpreter '" + interpreter + "'");
        }
        if (status == 130) {
            throw CliError(ExitCode::cancelled, "Python script was interrupted");
        }
        if (status != 0) {
            throw CliError(ExitCode::invalid_arguments,
                           "Python script failed with exit status " + std::to_string(status));
        }
        throw_if_interrupted();
        const std::vector<std::byte> output_bytes = read_input(staged.path(), options);
        const ctex::io::ProjectContainer output =
            ctex::io::read_project_container(output_bytes, project_limits(options)).container;
        static_cast<void>(measure_container(output, options));
        throw_if_interrupted();
        ctex::io::save_project_container_atomic(output_path, output);
        const std::uintmax_t published_bytes = std::filesystem::file_size(output_path);

        if (option_value(invocation, "--report") == "json") {
            std::cout << '{'
                      << standard_report_fields(invocation, executor, options, ExitCode::success,
                                                "ok", elapsed_milliseconds(started))
                      << ",\"outputs\":[{\"kind\":\"project\",\"path\":"
                      << json_string(output_path.string()) << ",\"bytes\":" << published_bytes
                      << "}],\"inputs\":[{\"kind\":\"document\",\"path\":"
                      << json_string(document_path.string())
                      << "},{\"kind\":\"script\",\"path\":" << json_string(script_path.string())
                      << "}],\"operations\":[\"read\",\"open\",\"script\",\"save\"]"
                      << ",\"document_asset\":" << json_string(document_identity)
                      << ",\"python\":" << json_string(interpreter) << "}\n";
        } else if (!invocation.quiet) {
            std::cout << "ran " << script_path.string() << " and wrote " << output_path.string()
                      << "\nexecutor: " << executor.selected << '\n';
        }
        return static_cast<int>(ExitCode::success);
    } catch (const CliError&) {
        throw;
    } catch (const ctex::io::ProjectContainerError& error) {
        const ExitCode code = error.code() == ctex::io::ProjectContainerErrorCode::over_limit
                                  ? ExitCode::over_budget
                                  : ExitCode::invalid_arguments;
        throw CliError(code, "could not run script: " + std::string(error.what()));
    } catch (const ctex::io::TextureDocumentIoError& error) {
        throw CliError(ExitCode::invalid_arguments,
                       "invalid texture document: " + std::string(error.what()));
    }
}

std::string export_outputs_json(const ctex::io::TextureExportResult& result,
                                const std::filesystem::path& output_directory) {
    std::string json{"["};
    for (std::size_t index = 0; index < result.buffers.size(); ++index) {
        if (index != 0) json += ',';
        const ctex::io::InMemoryTextureExport& output = result.buffers[index];
        json +=
            "{\"kind\":\"texture\",\"path\":" +
            json_string((output_directory / output.relative_path).string()) +
            ",\"bytes\":" + std::to_string(output.bytes.size()) +
            ",\"width\":" + std::to_string(output.width) +
            ",\"height\":" + std::to_string(output.height) +
            ",\"format\":" + json_string(ctex::io::export_image_format_name(output.format)) +
            ",\"color_space\":" + json_string(ctex::image::color_space_name(output.color_space)) +
            ",\"bit_depth\":" + std::to_string(static_cast<unsigned>(output.bit_depth)) + '}';
    }
    return json + ']';
}

int export_command(const Invocation& invocation, const SelectedExecutor& executor,
                   const ExecutionOptions& options, SteadyTime started) {
    const std::filesystem::path document_path{*option_value(invocation, "--document")};
    const std::filesystem::path output_directory{*option_value(invocation, "--output")};
    const std::string_view preset_identity = *option_value(invocation, "--preset");
    if (option_value(invocation, "--mesh").has_value()) {
        throw CliError(ExitCode::unsupported_operation,
                       "export mesh replacement is not implemented yet");
    }
    if (option_value(invocation, "--mesh-policy").has_value()) {
        throw CliError(ExitCode::invalid_arguments, "--mesh-policy requires --mesh");
    }
    const ctex::io::ExportPreset& preset = require_export_preset(preset_identity);
    validate_export_destination(output_directory);
    const std::vector<std::byte> project_bytes = read_input(document_path, options);
    try {
        ctex::io::ProjectContainer project =
            ctex::io::read_project_container(project_bytes, project_limits(options)).container;
        static_cast<void>(measure_container(project, options));
        const std::string document_identity = only_texture_document_identity(project);
        const ctex::doc::TextureDocument document = ctex::io::unpack_texture_document(
            project, document_identity, texture_document_limits(options));
        ctex::io::TextureExportOptions export_options;
        export_options.dry_run = true;
        const std::string project_name = document_path.stem().string();
        const ctex::io::TextureExportResult plan = ctex::io::export_texture_document_to_memory(
            project_name, document, preset, export_options);
        require_export_budget(document, plan.report, project_bytes.size(), options);
        export_options.dry_run = false;
        const ctex::io::TextureExportResult result = ctex::io::export_texture_document_to_memory(
            project_name, document, preset, export_options);
        throw_if_interrupted();
        StagedOutputDirectory staging(output_directory);
        for (const ctex::io::InMemoryTextureExport& output : result.buffers) {
            throw_if_interrupted();
            staging.write(output);
        }
        throw_if_interrupted();
        staging.publish();

        if (option_value(invocation, "--report") == "json") {
            std::cout << '{'
                      << standard_report_fields(invocation, executor, options, ExitCode::success,
                                                "ok", elapsed_milliseconds(started))
                      << ",\"outputs\":" << export_outputs_json(result, output_directory)
                      << ",\"inputs\":[{\"kind\":\"document\",\"path\":"
                      << json_string(document_path.string())
                      << "}],\"operations\":[\"read\",\"open\",\"plan\",\"encode\",\"publish\"]"
                      << ",\"document_asset\":" << json_string(document_identity)
                      << ",\"preset\":" << json_string(preset.identifier) << "}\n";
        } else if (!invocation.quiet) {
            std::cout << "exported " << result.buffers.size() << " textures to "
                      << output_directory.string() << "\nexecutor: " << executor.selected << '\n';
        }
        return static_cast<int>(ExitCode::success);
    } catch (const CliError&) {
        throw;
    } catch (const ctex::io::ProjectContainerError& error) {
        const ExitCode code = error.code() == ctex::io::ProjectContainerErrorCode::over_limit
                                  ? ExitCode::over_budget
                                  : ExitCode::invalid_arguments;
        throw CliError(code, "could not export document: " + std::string(error.what()));
    } catch (const ctex::io::TextureDocumentIoError& error) {
        throw CliError(ExitCode::invalid_arguments,
                       "invalid texture document: " + std::string(error.what()));
    } catch (const ctex::io::TextureExportError& error) {
        const ExitCode code = error.code() == ctex::io::TextureExportErrorCode::over_limit
                                  ? ExitCode::over_budget
                                  : ExitCode::invalid_arguments;
        throw CliError(code, "could not export textures: " + std::string(error.what()));
    } catch (const ctex::io::ExportPlanError& error) {
        throw CliError(ExitCode::invalid_arguments,
                       "could not plan texture export: " + std::string(error.what()));
    } catch (const ctex::io::ExportPresetError& error) {
        throw CliError(ExitCode::invalid_arguments,
                       "invalid export preset: " + std::string(error.what()));
    }
}

int dispatch(const Invocation& invocation) {
    const auto started = std::chrono::steady_clock::now();
    const SelectedExecutor executor = select_executor(invocation);
    const ExecutionOptions options = execution_options(invocation);
    try {
        throw_if_interrupted();
        if (invocation.command->name == "validate") {
            return validate_command(invocation, executor, options, started);
        }
        if (invocation.command->name == "info") {
            return info_command(invocation, executor, options, started);
        }
        if (invocation.command->name == "apply") {
            return apply_command(invocation, executor, options, started);
        }
        if (invocation.command->name == "export") {
            return export_command(invocation, executor, options, started);
        }
        if (invocation.command->name == "run") {
            return run_command(invocation, executor, options, started);
        }
        const std::string diagnostic = "command '" + std::string(invocation.command->name) +
                                       "' is not implemented yet (roadmap task 15.2)";
        if (option_value(invocation, "--report") == "json") {
            std::cout << '{'
                      << standard_report_fields(invocation, executor, options,
                                                ExitCode::unsupported_operation, "unsupported",
                                                elapsed_milliseconds(started))
                      << ",\"outputs\":[],\"inputs\":[],\"operations\":[],\"diagnostic\":"
                      << json_string(diagnostic) << "}\n";
        } else if (!invocation.quiet) {
            std::cout << "CyberTexel " << invocation.command->name
                      << "\nexecutor: " << executor.selected << '\n';
        }
        std::cerr << diagnostic << '\n';
        return static_cast<int>(ExitCode::unsupported_operation);
    } catch (const CliError& error) {
        std::cerr << error.what() << '\n';
        emit_error_report(invocation, executor, options, error, elapsed_milliseconds(started));
        return static_cast<int>(error.code());
    } catch (const std::exception& error) {
        const CliError internal(ExitCode::internal_error,
                                "internal error: " + std::string(error.what()));
        std::cerr << internal.what() << '\n';
        emit_error_report(invocation, executor, options, internal, elapsed_milliseconds(started));
        return static_cast<int>(internal.code());
    }
}

}  // namespace

int main(int argc, char** argv) {
    std::signal(SIGINT, handle_interrupt);
    std::signal(SIGTERM, handle_interrupt);
    if (argc == 2 && std::string_view(argv[1]) == "--help") {
        print_usage(std::cout);
        return static_cast<int>(ExitCode::success);
    }
    std::string error;
    const std::optional invocation = parse_invocation(argc, argv, error);
    if (!invocation.has_value()) {
        std::cerr << error << '\n';
        if (wants_json_report(argc, argv)) {
            emit_parse_failure_json(argc, argv, error);
        }
        return static_cast<int>(ExitCode::invalid_arguments);
    }
    return dispatch(*invocation);
}
