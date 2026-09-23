# The Linux package toolchain, so `just package-linux` produces and smoke-tests
# the same artifact on a developer's machine and in CI.
FROM ubuntu:24.04

ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
    && apt-get install --yes --no-install-recommends \
        ca-certificates \
        cmake \
        g++ \
        git \
        ninja-build \
        python3 \
    && rm -rf /var/lib/apt/lists/*
