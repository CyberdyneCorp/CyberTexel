# Windows and Android release follow-up

The first release scope ships and gates macOS, Linux and iOS. Windows and Android packages and the Android physical-device matrix remain planned work after the bootstrap change is archived. Keeping this as an active change preserves the requirement without describing unbuilt packages as shipped.

This change will add the Windows x64 and Android arm64 packages, link smoke tests, release gate evidence and a named Android device/API matrix. The existing Windows library, CLI and wheel CI coverage remains in place.
