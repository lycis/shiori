[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![Build](https://github.com/lycis/shiori/actions/workflows/build.yml/badge.svg)](https://github.com/lycis/shiori/actions/workflows/build.yml)
![Language: C23](https://img.shields.io/badge/language-C23-00599C?logo=c&logoColor=white)
![Platform: Windows-first](https://img.shields.io/badge/platform-Windows--first-0078D4?logo=windows11&logoColor=white)
![Status: Alpha](https://img.shields.io/badge/status-alpha-orange)

<div align="center">

<img src="shiori_header.png" alt="Shiori" width="420">

# Shiori

### A tiny personal scribe for thoughts that should not get away.

**Shiori** is a fast, local-first command-line scratchpad and task tracker written in C23.
Capture notes, manage todos, and review a colorful daily dashboard while keeping everything in Markdown files that remain yours.

No database. No account. No cloud. Just plain text.

</div>

<div align="center">
  <img src="docs/assets/shiori-demo.gif"
       alt="Shiori terminal demo"
       width="760">
</div>

---

## Why Shiori?

Most notes begin as a thought that needs to get out of your head before it disappears:

```console
shiori add --topic development investigate UTF-8 path handling
shiori todo add --due tomorrow prepare release notes "#work"
shiori today
```

For a longer session, stay in a focused capture prompt:

```console
shiori capture --topic planning
```

Shiori keeps the result in ordinary `NOTES.md` and `TODOS.md` files that work with any editor or Markdown tool, including Obsidian.

---

## Features

- 🦊 **Fast capture** — add one thought or collect many in an interactive session.
- 📝 **Plain Markdown** — daily notes and task lists remain readable without Shiori.
- 🪧 **Managed notes** — use stable IDs to inspect, remove, or retopic captured notes.
- 🏷️ **Topics and tags** — organize notes and search related notes and todos together.
- ✅ **Todo workflows** — track status, due dates, tags, and overdue work.
- 🌅 **Daily dashboard** — review a selected day's notes and active tasks.
- ✨ **Friendly terminal UI** — colored output, UTF-8 input, suggestions, and Tab completion.
- 🛡️ **Safe, extensible storage** — guarded file replacement plus local command hooks.
- 🔌 **Obsidian-friendly** — use a vault as `base_dir` without a plugin.

---

## Quick start

Download `shiori-0.2.0-windows-x64.zip` and its checksum from the
[0.2.0 release](https://github.com/lycis/shiori/releases/tag/0.2.0), verify the
download, extract `shiori.exe`, then run:

```console
.\shiori.exe help
.\shiori.exe init
.\shiori.exe add my first note
.\shiori.exe today
```

`init` creates `.shiori` in the current directory and stores `NOTES.md` and `TODOS.md` there by default.

Upgrading from 0.1.0 requires a one-time notes migration. Back up the workspace
and follow the [upgrade guide](docs/UPGRADING.md) before using 0.2.0 with
existing data.

---

## Build from source

You need GNU Make and a C23-capable compiler; the current supported development setup is Windows with Clang.

Shiori uses trunk-based development directly on `main`. The `main` branch contains
the current development state and is normally ahead of the latest user release;
it is not a stable release branch. To build a released version, check out its
version tag instead.

```console
make
```

This creates `shiori.exe`. Use `make release` for an optimized, statically linked executable or `make clean` to remove build output.

See **[Building Shiori](BUILD.md)** for toolchain requirements, all build and test targets, output locations, and the CI/release process.

---

## Commands at a glance

```text
shiori [options] <command> [options] [subcommand] ...
```

| Command | Description |
|---|---|
| `init` | Initialize a `.shiori` configuration. |
| `add` | Add a note to today's section (short form of `note add`). |
| `note` | Add, inspect, retopic, and remove notes by ID. |
| `capture` | Start an interactive session for rapidly capturing notes and todos. |
| `topic` | Browse notes and todos by topic or list topic statistics. |
| `tag` | Find notes and todos containing all specified tags. |
| `todo` | Add, list, update, and remove todos. |
| `today` | Show notes and active todos in a daily dashboard. |
| `config` | View the loaded configuration. |
| `console` | Start interactive console mode. |
| `util` | Run migrations and generate shell completion. |
| `help` | Show command help. |
| `version` | Display version and build information. |

Use the global `--debug` option for diagnostic output. Use `--no-color`, set `color: false` in `.shiori`, or define a non-empty `NO_COLOR` environment variable to disable ANSI colors. Redirected output is plain automatically; see the [user guide](docs/USER_GUIDE.md#color-output) for precedence details.

---

## Documentation

Shiori uses trunk-based development directly on `main`. Consequently, documentation
on `main` describes the current trunk and may cover behavior that has not been
released yet. Releases and their matching documentation are preserved by version
tags. Users of a released binary should select the guide for that binary's version
(shown by `shiori version`), rather than the guide on `main`.

| Version | User guide |
|---|---|
| Current trunk (`main`) | **[Trunk User Guide](docs/USER_GUIDE.md)** |
| 0.2.0 (latest release) | **[0.2.0 User Guide](https://github.com/lycis/shiori/blob/0.2.0/docs/USER_GUIDE.md)** |
| 0.1.0 | **[0.1.0 User Guide](https://github.com/lycis/shiori/blob/0.1.0/docs/USER_GUIDE.md)** |

- **[Upgrade Guide](docs/UPGRADING.md)** — backups, migrations, verification, and recovery between releases
- **[Changelog](CHANGELOG.md)** — release history, compatibility notes, features, and fixes
- **[Build Guide](BUILD.md)** — toolchain setup, local build targets, tests, CI, and release packaging
- **[Hooks Guide](docs/HOOKS.md)** — hook lifecycle, environment variables, examples, security, and limitations
- **[Contributing](CONTRIBUTING.md)** — development setup and project conventions
- **[Security](SECURITY.md)** — supported versions and private vulnerability reporting

---

## Status

Shiori is alpha software, currently Windows-first, and developed with Clang + C23. Linux abstractions exist, but Linux support is incomplete. Current ideas and planned work live in [`planning/IDEAS.md`](planning/IDEAS.md).

The project will stay focused on quick capture rather than becoming a full knowledge-management platform wearing a tiny CLI hat.

---

## Contributing and license

Contributions, bug reports, and ideas are welcome. Shiori is available under the [`MIT License`](LICENSE).

---

<div align="center">

**Write it down. Keep moving.**

🦊

</div>
