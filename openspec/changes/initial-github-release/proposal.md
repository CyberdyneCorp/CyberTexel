## Why

The first-release packages exist, but CI is red and no GitHub release has been published. A release must be assembled from verified CI artifacts, with the physical iPad gate run on the named device. Windows and Android remain in their separate follow-up change.

## What Changes

- Repair failing CI checks on Linux, macOS and iOS without weakening their assertions.
- Run the reference-device gate successfully on the named Mac and iPad.
- Assemble and verify the three in-scope package archives from the release commit.
- Publish the initial `v0.1.0` GitHub release with those archives and accurate release notes.

## Scope

Windows and Android packages, device validation and release assets are excluded.
