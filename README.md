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

---

## Why Shiori?

Most notes do not begin as documents. They begin as a thought that needs to get out of your head before it disappears:

```console
shiori add --topic development investigate UTF-8 path handling
```

Shiori writes it under today's heading in `NOTES.md`:

```markdown
# 2026-08-13
* investigate UTF-8 path handling #shiori/topic/development
```

Tasks are just as quick:

```console
shiori todo add --due tomorrow prepare release notes "#work"
shiori todo start 1
```

Then `shiori today` brings the day's notes and active todos together in a styled terminal dashboard.

Your data remains ordinary Markdown that works with any text editor or Markdown tool, including Obsidian.

---

## Features

- 🦊 **Quick capture** — add a thought directly from the command line.
- 📝 **Plain Markdown** — your notes and tasks stay readable without Shiori.
- 📅 **Automatic daily sections** — notes are grouped under `# YYYY-MM-DD` headings.
- 🪧 **Note topics** — organize related notes and review them across daily sections.
- ✅ **Todo tracking** — move stable task IDs through open, in-progress, and done states.
- ⏰ **Due dates** — schedule tasks and surface due or overdue work in the dashboard.
- 🏷️ **Flexible filtering** — find todos by status or one or more Markdown tags.
- ✏️ **Task maintenance** — rewrite, remove, reopen, or prune completed tasks.
- 🌅 **Daily dashboard** — review notes and active work for today or another selected date.
- 💻 **Interactive console** — keep Shiori open while capturing several thoughts.
- 🛡️ **Safe updates** — file rewrites use temporary files with backup and restore handling.
- 🔌 **Obsidian-friendly** — point `base_dir` at a vault; no plugin is required.

---

## Quick start

Every successful GitHub Actions build publishes a `shiori-windows-x64` artifact containing the standalone `shiori.exe`. Download and extract it, then run:

```console
.\shiori.exe help
.\shiori.exe init
.\shiori.exe add my first note
.\shiori.exe today
```

`shiori init` creates a `.shiori` configuration in the current directory and uses that directory for `NOTES.md` and `TODOS.md` by default.

For installation details and complete usage examples, see the **[Shiori User Guide](docs/USER_GUIDE.md)**.

---

## Build from source

You need a C23-capable compiler, GNU Make, and currently a Windows development environment with Clang.

```console
make
```

This creates `shiori.exe`. Other useful targets are:

```console
make release
make clean
```

The release executable statically links the C runtime and requires no third-party DLLs.

---

## Commands at a glance

```text
shiori [options] <command> [options] [subcommand] ...
```

| Command | Description |
|---|---|
| `init` | Initialize a `.shiori` configuration. |
| `add` | Add a note to today's section. |
| `topic` | Browse notes by topic or list topic statistics. |
| `todo` | Add, list, update, and remove todos. |
| `today` | Show notes and active todos in a daily dashboard. |
| `config` | View the loaded configuration. |
| `console` | Start interactive console mode. |
| `help` | Show command help. |
| `version` | Display version and build information. |

Use the global `--debug` option to show diagnostic output.

See the **[User Guide](docs/USER_GUIDE.md)** for topics, due dates, the todo command reference, filters, date selection, configuration, data formats, and troubleshooting.

---

## Status

Shiori is young and actively evolving. The current implementation is **Windows-first** and developed with **Clang + C23**. Some Windows and Linux platform abstractions exist, but Linux support is not complete.

Current areas being explored include:

- complete Linux support
- versioned releases and easier installation
- configurable note ordering
- richer note and todo workflows beyond the current topics, due dates, filtering, and editing features
- additional configuration commands
- improved interactive console parsing
- safer and more portable filesystem abstractions

The project will stay focused on quick capture rather than becoming a full knowledge-management platform wearing a tiny CLI hat.

---

## Philosophy

**Local first.** Your thoughts should not require a network connection.

**Plain text.** Markdown is durable, portable, and easy to inspect.

**Fast capture over organization.** Get the thought out of your head first. Structure can come later.

**Small software.** Shiori is intentionally compact and dependency-free.

**Interoperability over lock-in.** If you stop using Shiori tomorrow, your notes and todos are still Markdown.

---

## Contributing and license

Contributions, bug reports, and ideas are welcome. See [`CONTRIBUTING.md`](CONTRIBUTING.md) for development setup and project conventions.

Shiori is available under the [`MIT License`](LICENSE).

---

<div align="center">

**Write it down. Keep moving.**

🦊

</div>
