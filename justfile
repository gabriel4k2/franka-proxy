set windows-shell := ["powershell.exe", "-NoLogo", "-NoProfile", "-Command"]

build_dir := if os() == "windows" { "build" } else { "out/build/linux-release" }

# Rebuild the configured source tree, including the proxy and Python binding.
build:
    cmake --build "{{build_dir}}" --config Release --parallel 2

# Hardware-free timing checks only; never starts the proxy or remote controller.
test:
    cmake --build "{{build_dir}}" --config Release --target franka_sequence_timing_test --parallel 2
    ctest --test-dir "{{build_dir}}" -C Release -R '^sequence_timing_offline$' --output-on-failure
