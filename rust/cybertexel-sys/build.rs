use std::env;
use std::fs;
use std::path::{Path, PathBuf};
use std::process::Command;

fn run(command: &mut Command) {
    let status = command
        .status()
        .expect("failed to start native build command");
    assert!(status.success(), "native build command failed: {command:?}");
}

fn library_directory(build: &Path, target_os: &str) -> PathBuf {
    if target_os == "windows" {
        build.join("Release")
    } else {
        build.to_path_buf()
    }
}

fn copy_runtime_library(directory: &Path, out_dir: &Path, target_os: &str) {
    let names: Vec<_> = fs::read_dir(directory)
        .expect("failed to inspect native library directory")
        .filter_map(Result::ok)
        .filter_map(|entry| entry.file_name().into_string().ok())
        .filter(|name| match target_os {
            "macos" => name.starts_with("libcybertexel_c") && name.ends_with(".dylib"),
            "linux" => name.starts_with("libcybertexel_c.so"),
            "windows" => name == "cybertexel_c.dll",
            _ => false,
        })
        .collect();
    let profile = out_dir
        .ancestors()
        .nth(3)
        .expect("Cargo OUT_DIR does not contain a profile directory");
    for destination in [profile.to_path_buf(), profile.join("deps")] {
        fs::create_dir_all(&destination).expect("failed to create Cargo runtime directory");
        for name in &names {
            fs::copy(directory.join(name), destination.join(name))
                .expect("failed to copy native runtime library");
        }
    }
}

fn main() {
    let manifest = PathBuf::from(env::var_os("CARGO_MANIFEST_DIR").unwrap());
    let repository = manifest.join("../..").canonicalize().unwrap();
    let out_dir = PathBuf::from(env::var_os("OUT_DIR").unwrap());
    let target_os = env::var("CARGO_CFG_TARGET_OS").unwrap();
    let target = env::var("TARGET").unwrap();
    let build = manifest.join("../target/cybertexel-native").join(target);
    let configured = env::var_os("CYBERTEXEL_LIBRARY_DIR").map(PathBuf::from);
    let library = if let Some(directory) = configured {
        directory
    } else {
        run(Command::new("cmake").args([
            "-S",
            repository.to_str().unwrap(),
            "-B",
            build.to_str().unwrap(),
            "-DBUILD_TESTING=OFF",
            "-DCMAKE_BUILD_TYPE=Release",
        ]));
        run(Command::new("cmake").args([
            "--build",
            build.to_str().unwrap(),
            "--config",
            "Release",
            "--target",
            "cybertexel_c",
        ]));
        library_directory(&build, &target_os)
    };

    println!("cargo:rustc-link-search=native={}", library.display());
    println!("cargo:rustc-link-lib=dylib=cybertexel_c");
    println!("cargo:rerun-if-env-changed=CYBERTEXEL_LIBRARY_DIR");
    println!(
        "cargo:rerun-if-changed={}",
        repository.join("VERSION").display()
    );
    for path in ["CMakeLists.txt", "cmake", "src", "thirdparty"] {
        println!("cargo:rerun-if-changed={}", repository.join(path).display());
    }
    println!(
        "cargo:rerun-if-changed={}",
        repository.join("include/ctex/capi.h").display()
    );
    copy_runtime_library(&library, &out_dir, &target_os);
}
