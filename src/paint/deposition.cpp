#include <algorithm>
#include <cmath>
#include <ctex/paint/deposition.hpp>
#include <limits>
#include <stdexcept>
#include <vector>

namespace ctex::paint {
namespace {

bool finite(double value) { return std::isfinite(value); }

std::size_t checked_texel_count(std::uint32_t width, std::uint32_t height) {
    if (width == 0 || height == 0 ||
        static_cast<std::size_t>(width) >
            std::numeric_limits<std::size_t>::max() / static_cast<std::size_t>(height)) {
        throw std::invalid_argument("deposition dimensions are invalid");
    }
    return static_cast<std::size_t>(width) * height;
}

void validate_mode(DepositionMode mode) {
    if (mode != DepositionMode::non_building && mode != DepositionMode::build_up) {
        throw std::invalid_argument("deposition mode is invalid");
    }
}

DepositionRaster make_result(std::uint32_t width, std::uint32_t height, DepositionMode mode) {
    validate_mode(mode);
    const std::size_t texel_count = checked_texel_count(width, height);
    return {.width = width,
            .height = height,
            .mode = mode,
            .non_building_coverage = std::vector<double>(texel_count, 0.0),
            .build_up_deposition = std::vector<double>(texel_count, 0.0),
            .strength = std::vector<double>(texel_count, 0.0),
            .applied_stamp_count = 0};
}

void validate_event(const RejectedStampCoverage& event, std::size_t texel_count,
                    std::size_t stamp_count) {
    if (event.stamp_ordinal >= stamp_count || event.values.size() != texel_count ||
        !std::all_of(event.values.begin(), event.values.end(),
                     [](double value) { return finite(value) && value >= 0.0 && value <= 1.0; })) {
        throw std::invalid_argument("deposition stamp event is invalid");
    }
}

void apply_non_building(const Stamp& stamp, std::span<const double> event,
                        DepositionRaster& result) {
    for (std::size_t texel = 0; texel < event.size(); ++texel) {
        const double coverage = event[texel];
        result.non_building_coverage[texel] =
            std::max(result.non_building_coverage[texel], coverage);
        result.strength[texel] =
            std::max(result.strength[texel], stamp.opacity * stamp.flow * coverage);
    }
}

void apply_build_up(const Stamp& stamp, std::span<const double> event, DepositionRaster& result) {
    for (std::size_t texel = 0; texel < event.size(); ++texel) {
        const double previous = result.build_up_deposition[texel];
        const double deposition = 1.0 - (1.0 - previous) * (1.0 - stamp.flow * event[texel]);
        result.build_up_deposition[texel] = deposition;
        result.strength[texel] = std::max(result.strength[texel], stamp.opacity * deposition);
    }
}

}  // namespace

StrokeDepositionAccumulator::StrokeDepositionAccumulator(const ResolvedStroke& stroke,
                                                         std::uint32_t width, std::uint32_t height,
                                                         DepositionMode mode)
    : stroke_(ingest_resolved_stroke(stroke)), result_(make_result(width, height, mode)) {}

void StrokeDepositionAccumulator::apply(std::span<const RejectedStampCoverage> stamp_events) {
    std::uint64_t simulated_next = next_stamp_ordinal_;
    std::vector<const RejectedStampCoverage*> new_events;
    new_events.reserve(stamp_events.size());
    for (const RejectedStampCoverage& event : stamp_events) {
        validate_event(event, result_.strength.size(), stroke_.stamps.size());
        if (event.stamp_ordinal < simulated_next) {
            continue;
        }
        if (event.stamp_ordinal != simulated_next) {
            throw std::invalid_argument("deposition stamp events must be contiguous and ordered");
        }
        new_events.push_back(&event);
        ++simulated_next;
    }
    for (const RejectedStampCoverage* event : new_events) {
        const Stamp& stamp = stroke_.stamps[event->stamp_ordinal];
        if (result_.mode == DepositionMode::non_building) {
            apply_non_building(stamp, event->values, result_);
        } else {
            apply_build_up(stamp, event->values, result_);
        }
        ++result_.applied_stamp_count;
    }
    next_stamp_ordinal_ = simulated_next;
}

DepositionRaster evaluate_deposition(const ResolvedStroke& stroke,
                                     const RejectedCoverageRaster& rejected, DepositionMode mode) {
    StrokeDepositionAccumulator accumulator(stroke, rejected.coverage.width,
                                            rejected.coverage.height, mode);
    if (rejected.coverage.values.size() != accumulator.result().strength.size()) {
        throw std::invalid_argument("rejected coverage dimensions are inconsistent");
    }
    accumulator.apply(rejected.stamp_events);
    if (accumulator.next_stamp_ordinal() != stroke.stamps.size()) {
        throw std::invalid_argument("rejected coverage is missing canonical stamp events");
    }
    return accumulator.result();
}

}  // namespace ctex::paint
