# Dependency and licence policy

CyberTexel may ship code under MIT, BSD-2-Clause, BSD-3-Clause, Apache-2.0,
MPL-2.0 or Zlib terms. A dependency under GPL, LGPL, AGPL or an unreviewed
licence must not be linked into a distributed library or tool.

The audit covers dependencies compiled into shipped artifacts, regardless of
how they enter the build:

- source trees under `thirdparty/`, `vendor/` or `external/`;
- CMake `FetchContent_Declare` dependencies;
- packages introduced with CMake `find_package`.

Every discovered dependency must have an entry in
`thirdparty/dependencies.json`. The entry records its SPDX licence identifier,
source URL, immutable revision, discovery name and a repository copy of the
licence text. The same name, licence and revision must appear in
`THIRD_PARTY_NOTICES.md`. Package-only dependencies still require a copied
licence text so the attribution is reviewable without network access.

Run `just gate-licence` after adding or changing a dependency. An untracked
tree or CMake declaration fails by name; a forbidden licence fails before the
dependency can be treated as release-ready.
