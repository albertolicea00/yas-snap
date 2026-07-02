# CLAUDE.md — YAS Snap

## What
Native GUI wrapper for **Snapcraft** (`snap`). Part of YAS suite.
Status: **scaffolded & unit-tested** — vendored core + adapter + QML shell compile, 3/3 QtTest suites pass (verified cross-compiling on macOS). Pending: build + QA on the real target platform.

## Stack
- C++20 + Qt 6.7+ (Qt Quick / QML), CMake ≥ 3.24, GCC/Clang
- Native windowing via Qt QPA plugins: **wayland** with **xcb** (X11) fallback.
- CLI execution: `QProcess` wrapping `snap`. Never bundle it (snapd required).
- Architecture: **vendored core copy** (identical across suite, NO shared library by design) + `snap` adapter. Master template: `../yas-core/` local folder (not published). Core fixes must be replicated across repos.

## Target platform
Ubuntu primary; any distro with snapd. x64 + arm64.

## snap specifics
- Mutating ops need root **or** polkit via snapd — snapd exposes a REST API over UNIX socket (`/run/snapd.socket`) with polkit integration. Prefer the REST API (JSON!) over CLI scraping where possible; CLI remains for the terminal log view.
- Key commands: `snap find`, `snap info`, `snap list`, `snap install/remove/refresh`, `snap refresh --hold` (pin equivalent), `snap revert`, `snap switch --channel`, `snap connections`.
- Channels (stable/candidate/beta/edge) + tracks are core UX — UI needs channel selector.
- Revert (rollback to previous revision) is a differentiator vs other managers — expose it.
- `snap refresh` auto-runs in background system-wide; UI should show pending/held refreshes, not assume it controls all updates.

## Design (see DESIGN.md)
- Dark theme. Base `#222629`, accent **Rust `#822007`**, highlight `#8220071A`, text `#F8F8F2` / `#ACADAD`.
- App tag: **SNAP**. Fonts: Outfit/Inter (UI), Fira Code or JetBrains Mono (CLI output).
- DESIGN.md previously labeled the accent "Purple"; hex kept, label corrected to Rust.

## Conventions
- Conventional Commits (no co-author attribution), feature branches, PRs per CONTRIBUTING.md. Never push to origin without explicit ask.
- Planned layout (mirrors yas-brew, the reference scaffold): `src/core/` (vendored), `src/snapadapter.*`, `src/main.cpp`, `qml/core/` (vendored) + `qml/Main.qml`, `tests/`, `assets/fonts/`, `icons/` (exists), CMakeLists.txt + CMakePresets.json.
- Packaging: snap package (dogfooding) — note Qt runtime inside snap needs core22/core24 + kde-neon extension.

## Key files
README.md · DESIGN.md · CONTRIBUTING.md · EULA.md · SECURITY.md · icons/
