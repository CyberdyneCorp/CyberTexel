# Reproducible builds

`just gate-reproducible` builds the shipped C library twice from the same commit
into two separate trees with one toolchain and compares every produced artifact
byte for byte. An artifact that differs fails the gate unless this document
names the input that makes it differ; the gate reads the table below, so an
undocumented difference cannot pass and a documented one cannot be a bare
exemption.

## Fixed inputs

The build does not embed a timestamp, a hostname, a user name or a build
counter. The library version comes from [`VERSION`](../VERSION), and every
generated source is written from committed data. Archives are written with a
fixed ZIP timestamp so a package is a function of its contents.

## Documented variances

| Artifact | Differing input | Why it is unavoidable |
| --- | --- | --- |

No variance is currently documented: on the `headless` preset the two builds
produce identical libraries. An entry here is a statement that a specific
toolchain writes a specific irreproducible input, not a general allowance.

## Scope

The gate runs on the `headless` preset by default and accepts `--preset` for the
shipped platform presets. Reproducibility is a property of one toolchain: two
different compilers, or two different versions of one compiler, are not expected
to agree, and the gate never compares across them.
