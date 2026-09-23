#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace ctex::io::test {

constexpr std::uint32_t icc_signature(char a, char b, char c, char d) noexcept {
    return (static_cast<std::uint32_t>(static_cast<unsigned char>(a)) << 24U) |
           (static_cast<std::uint32_t>(static_cast<unsigned char>(b)) << 16U) |
           (static_cast<std::uint32_t>(static_cast<unsigned char>(c)) << 8U) |
           static_cast<std::uint32_t>(static_cast<unsigned char>(d));
}

inline void write_icc_u32(std::vector<unsigned char>& profile, std::size_t offset,
                          std::uint32_t value) {
    profile[offset] = static_cast<unsigned char>(value >> 24U);
    profile[offset + 1] = static_cast<unsigned char>(value >> 16U);
    profile[offset + 2] = static_cast<unsigned char>(value >> 8U);
    profile[offset + 3] = static_cast<unsigned char>(value);
}

inline void write_icc_fixed(std::vector<unsigned char>& profile, std::size_t offset, double value) {
    write_icc_u32(profile, offset,
                  static_cast<std::uint32_t>(static_cast<std::int32_t>(value * 65'536.0 + 0.5)));
}

inline std::vector<unsigned char> make_rec709_icc_profile(bool linear) {
    constexpr std::array xyz_offsets{216U, 236U, 256U, 276U};
    constexpr std::array trc_offsets{296U, 328U, 360U};
    constexpr std::array xyz_signatures{
        icc_signature('r', 'X', 'Y', 'Z'), icc_signature('g', 'X', 'Y', 'Z'),
        icc_signature('b', 'X', 'Y', 'Z'), icc_signature('w', 't', 'p', 't')};
    constexpr std::array trc_signatures{icc_signature('r', 'T', 'R', 'C'),
                                        icc_signature('g', 'T', 'R', 'C'),
                                        icc_signature('b', 'T', 'R', 'C')};
    constexpr std::array<std::array<double, 3>, 4> xyz_values{
        std::array{0.4361, 0.2225, 0.0139}, std::array{0.3851, 0.7169, 0.0971},
        std::array{0.1431, 0.0606, 0.7142}, std::array{0.9642, 1.0, 0.8249}};
    std::vector<unsigned char> profile(392);
    write_icc_u32(profile, 0, static_cast<std::uint32_t>(profile.size()));
    write_icc_u32(profile, 12, icc_signature('m', 'n', 't', 'r'));
    write_icc_u32(profile, 16, icc_signature('R', 'G', 'B', ' '));
    write_icc_u32(profile, 20, icc_signature('X', 'Y', 'Z', ' '));
    write_icc_u32(profile, 36, icc_signature('a', 'c', 's', 'p'));
    write_icc_u32(profile, 128, 7);
    std::size_t table = 132;
    for (std::size_t index = 0; index < xyz_offsets.size(); ++index) {
        write_icc_u32(profile, table, xyz_signatures[index]);
        write_icc_u32(profile, table + 4, xyz_offsets[index]);
        write_icc_u32(profile, table + 8, 20);
        table += 12;
        write_icc_u32(profile, xyz_offsets[index], icc_signature('X', 'Y', 'Z', ' '));
        for (std::size_t component = 0; component < 3; ++component) {
            write_icc_fixed(profile, xyz_offsets[index] + 8 + component * 4,
                            xyz_values[index][component]);
        }
    }
    constexpr std::array srgb_curve{2.4, 1.0 / 1.055, 0.055 / 1.055, 1.0 / 12.92, 0.04045};
    for (std::size_t index = 0; index < trc_offsets.size(); ++index) {
        write_icc_u32(profile, table, trc_signatures[index]);
        write_icc_u32(profile, table + 4, trc_offsets[index]);
        write_icc_u32(profile, table + 8, linear ? 12U : 32U);
        table += 12;
        if (linear) {
            write_icc_u32(profile, trc_offsets[index], icc_signature('c', 'u', 'r', 'v'));
            write_icc_u32(profile, trc_offsets[index] + 8, 0);
        } else {
            write_icc_u32(profile, trc_offsets[index], icc_signature('p', 'a', 'r', 'a'));
            profile[trc_offsets[index] + 9] = 3;
            for (std::size_t parameter = 0; parameter < srgb_curve.size(); ++parameter) {
                write_icc_fixed(profile, trc_offsets[index] + 12 + parameter * 4,
                                srgb_curve[parameter]);
            }
        }
    }
    return profile;
}

}  // namespace ctex::io::test
