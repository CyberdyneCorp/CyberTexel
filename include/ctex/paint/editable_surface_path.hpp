#ifndef CTEX_PAINT_EDITABLE_SURFACE_PATH_HPP
#define CTEX_PAINT_EDITABLE_SURFACE_PATH_HPP

#include <ctex/doc/editable_authoring.hpp>
#include <ctex/paint/stroke.hpp>
#include <vector>

namespace ctex::paint {

[[nodiscard]] std::vector<StrokeInputSample> editable_surface_path_samples(
    const doc::EditableAuthoringEntry& entry);
[[nodiscard]] ResolvedStroke evaluate_editable_surface_path(
    const doc::EditableAuthoringEntry& entry, StrokeSettings settings = {});

}  // namespace ctex::paint

#endif
