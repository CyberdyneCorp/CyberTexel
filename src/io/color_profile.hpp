#pragma once

#include <ctex/io/image_io.hpp>
#include <string>
#include <vector>

namespace ctex::io::detail {

struct ResolvedProfileColorSpace {
    image::ColorSpace color_space;
    ColorSpaceSource source;
    std::vector<std::string> diagnostics;
};

[[nodiscard]] ResolvedProfileColorSpace resolve_psd_color_space(const DecodeRequest& request);

}  // namespace ctex::io::detail
