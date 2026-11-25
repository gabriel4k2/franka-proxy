# Panda joint streaming

Send joint targets from Python. The server uses Ruckig to follow the newest target
through one persistent libfranka control session.

**Experimental: not validated on a robot or built on Windows.**

Stop the legacy `franka_proxy` before using this server. They must not control the
robot together.

## Build and test

Use the lab's existing vcpkg installation and a robot-compatible libfranka.
Required packages: `franka`, `asio`, `nlohmann-json` and **Ruckig 0.15.3**.

Install Ruckig once, using the same triplet as libfranka:

```text
vcpkg install "ruckig[core]" --classic
```

For the Windows lab installation, specify `"ruckig[core]:x64-windows"`.
If its port offers another version, resolve that first; do not relax the version
check.

From this repository's root, configure with the existing toolchain:

```text
make streaming-configure VCPKG_TOOLCHAIN="/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake"
make streaming-test
```

This builds the server and runs offline control, network and Python-client tests.
**It does not connect to the robot.** Offline tests do not establish safe motion
or correct hardware timing.

Build output:

- Linux: `out/build/streaming`
- Windows: `build-streaming` (`Release` for the Visual Studio generator)

The executable's `--help` is also hardware-free. Normal startup connects to the
robot. On Windows, check dependency DLL deployment before hardware use.

## Use from Python

Add `streaming/python` to `PYTHONPATH`. Import `FrankaStream` from `franka_stream`.
It uses only Python's standard library.

The client connects to the **server PC**, normally `127.0.0.1:5000`, not the robot's
FCI address. Only one client connection is supported.

Constructing the client connects but does not start control.

| Method          | What it does                                  |
| --------------- | --------------------------------------------- |
| `start()`       | Begin control at rest; no homing              |
| `set_target(q)` | Submit seven absolute joint angles in radians |
| `get_state()`   | Read measured joints, status and diagnostics  |
| `stop()`        | Brake and end control                         |
| `close()`       | Stop if started, then close the connection    |

Normal context-manager exit also closes the client. After stopping, call `start()`
explicitly before sending more targets. There is no automatic error recovery.

**Targets replace each other; they are not queued.** Pace your updates rather
than submitting a trajectory in a tight loop. The server owns the 1 kHz loop.

Acceptance is not arrival. Even the state's `sequence` only reports a consumed
target. Monitor state for faults; do not count acknowledgements as completed motion.
The client closes after ambiguous I/O failures rather than retrying.

## Limits and stopping

- Each target joint must stay within the Panda limits plus a 0.01 rad margin,
  and within **0.2 rad** of its latest measured position.
- Targets are rejected if published state is older than **100 ms**.
- After **0.5 s without a valid target**, the server brakes and holds. A fresh
  target can resume tracking. State polling does not refresh this watchdog.
- Stop or disconnect requests braking and session termination. A process crash
  or lost robot connection cannot guarantee software braking.

Motion limits are fixed in [control.hpp](src/control.hpp).
**The legacy proxy's `speed_factor` has no effect.** Joint checks are not collision
avoidance or proof of stopping clearance.

Realtime scheduling is enforced by default. See `--help` for server options.
The protocol has no authentication; keep it on a trusted, isolated network.

## Reusing FK/IK

The existing math helpers work without running the legacy proxy. Do not construct
its remote controller alongside streaming, even just to obtain state.

`ik_fast_closest` expects a base-to-joint-7 transform, not a TCP transform:

```text
target_base_T_j7 = target_base_T_tcp × tcp_T_j7
```

Use the calibrated tool transform and fresh measured joints as the IK seed.
Repository defaults are not proof of the mounted tool's calibration.
