FROM ubuntu:24.04

ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
    && apt-get install --yes --no-install-recommends \
        ca-certificates \
        clang \
        cmake \
        git \
        libclang-rt-18-dev \
        libfuzzer-18-dev \
        llvm-18 \
        ninja-build \
        python3 \
    && rm -rf /var/lib/apt/lists/*
