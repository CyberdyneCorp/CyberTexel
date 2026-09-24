# Design

Use the existing `release/platforms.json` manifest and packaging recipes as the source of truth. Advance Windows and Android from deferred to in scope only after their archives link and smoke test. Add physical Android device runs with recorded device/API identities; an iPad result cannot substitute. Expand the platform-package requirement to all five platforms when these gates pass.
