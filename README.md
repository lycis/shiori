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

Most notes do not begin as documents.

They begin as:

> remember to check that API  
> idea: use daily headings  
> buy printer paper  
> this bug is probably the parser

Opening a note-taking application, finding the right page, and formatting the thought is often enough friction to make the thought disappear.

Shiori is built for the opposite workflow:

```console
shiori add investigate UTF-8 path handling
```

Done.

The note is written to your Markdown file under today's heading:

```markdown
# 2026-08-12
* investigate UTF-8 path handling
```

Add another thought later:

```console
shiori add check whether fopen_s is available on Linux
```

and Shiori appends it to the same day:

```markdown
# 2026-08-12
* investigate UTF-8 path handling
* check whether fopen_s is available on Linux
```

The result is a lightweight chronological stream of notes that can be opened with any text editor or Markdown tool, including Obsidian.

---

## Features

- 🦊 **Quick capture** — add a thought directly from the command line.
- 📝 **Plain Markdown** — your notes stay readable without Shiori.
- 📅 **Automatic daily sections** — notes are grouped under `# YYYY-MM-DD` headings.
- 📂 **Configurable storage location** — keep `NOTES.md` and `TODOS.md` wherever you want.
- 🛡️ **Safe file replacement** — note and todo updates use temporary files with backup/restore handling.
- 💻 **Interactive console** — keep Shiori open while capturing several thoughts.
- ✅ **Markdown todo tracking** — add tasks with stable IDs and move them through open, in-progress, and done states.
- 🏷️ **Flexible todo views** — filter tasks by status or by one or more Markdown tags.
- ✏️ **Task maintenance** — rewrite, remove, reopen, or prune completed todos from the command line.
- 🌅 **Daily dashboard** — see a styled overview of a day's notes and active work.
- ⏪ **Date selection** — inspect today, yesterday, tomorrow, or any `YYYY-MM-DD` date.
- 🔍 **Debug mode** — inspect Shiori's internal plumbing when something looks suspicious.
- 🌱 **Small by design** — a deliberately compact, dependency-free C23 utility.
- 🔌 **Obsidian-friendly** — point `base_dir` at an Obsidian vault and the generated Markdown remains ordinary vault content.

---

## Status

Shiori is young and actively evolving.

The current implementation is **Windows-first** and developed with **Clang + C23**. The code already contains some platform abstractions for Windows and Linux, but Linux support is not complete yet.

Expect sharp edges, changing configuration options, and the occasional fox footprint in the plumbing.

---

## Quick start

### Download the standalone executable

Every successful GitHub Actions build publishes a `shiori-windows-x64`
artifact containing `shiori.exe`. Download the artifact, extract it, and run:

```console
.\shiori.exe help
```

The release executable statically links the C runtime. It does not require a
separate Visual C++ Redistributable or any third-party DLLs; it only uses DLLs
provided by Windows itself.

### 1. Build Shiori

Clone the repository and build it manually.

You need:

- a C23-capable compiler
- GNU Make
- currently, a Windows development environment with Clang

Then run:

```console
make
```

This creates:

```text
shiori.exe
```

For a release build:

```console
make release
```

To remove build output:

```console
make clean
```

> Package-manager installation and prebuilt releases are planned for later. For now, Shiori is installed manually.

### 2. Put Shiori on your `PATH`

You can either run Shiori directly from the project directory:

```console
.\shiori.exe help
```

or copy `shiori.exe` to a directory that is already part of your `PATH`.

For example, you might create:

```text
C:\Tools\shiori\
```

copy `shiori.exe` there, and add that directory to your user `PATH`.

After that:

```console
shiori help
```

should work from any terminal.

### 3. Initialize Shiori

Run:

```console
shiori init
```

Shiori creates a `.shiori` configuration file in the current directory.

The generated configuration currently looks roughly like this:

```yaml
# Initialized: ...
version: 1
base_dir: C:\path\to\your\notes
```

By default, `base_dir` is the directory from which you ran `shiori init`.

If you already have a configuration and intentionally want to recreate it:

```console
shiori init --reinit
```

---

## Usage

```text
shiori [options] <command> [options] [subcommand] ...
```

### Global options

| Option | Description |
|---|---|
| `--debug` | Show debug and plumbing output. |

### Commands

| Command | Description |
|---|---|
| `init` | Initialize a new `.shiori` configuration. |
| `add` | Add a note or thought to today's section. |
| `todo` | Add, list, update, and remove todos. |
| `today` | Show notes and active todos in a daily dashboard. |
| `config` | View and manage configuration. |
| `console` | Start interactive console mode. |
| `help` | Show command help. |
| `version` | Display version and build information. |

---

## Adding notes

The core Shiori workflow is intentionally boring:

```console
shiori add remember to review the architecture diagram
```

If today's section already exists in `NOTES.md`, Shiori adds the note to that block.

If it does not exist, Shiori creates it.

Example:

```markdown
# 2026-08-12
* remember to review the architecture diagram
* investigate SQLite later, but probably do not need it

# 2026-08-11
* rename the project
```

Shiori currently stores notes as Markdown bullet points using `*`.

---

## Managing todos

Todos live alongside notes as ordinary Markdown, but in their own `TODOS.md` file. Add a task with:

```console
shiori todo add prepare release notes #work
```

Each task receives a stable numeric ID and creation date. Use the ID to move it through its lifecycle:

```console
shiori todo start 1
shiori todo done 1
shiori todo reopen 1
```

List open and in-progress work (the default view):

```console
shiori todo list
```

Status and tag filters make larger lists easier to scan:

```console
shiori todo list --open
shiori todo list --in-progress
shiori todo list --done
shiori todo list --all
shiori todo list --tag work
shiori todo list --tag work --tag urgent
shiori todo list --done --tag work
```

Multiple status switches combine; multiple `--tag` values require a todo to contain every selected tag.

You can also edit or permanently remove individual tasks:

```console
shiori todo rewrite 1 prepare final release notes
shiori todo remove 1
```

To remove every completed task, first preview the count, then confirm explicitly:

```console
shiori todo prune
shiori todo prune --force
```

Run `shiori todo --help` or `shiori todo list --help` for the built-in reference.

---

## Daily dashboard

`today` combines notes for a selected date with your current open and in-progress todos in a colorful terminal dashboard:

```console
shiori today
```

Review another day with an ISO date or a convenient relative selector:

```console
shiori today --date 2026-08-12
shiori today --date yesterday
shiori today --date tomorrow
shiori today --date today
```

The note section follows the selected date. The todo sections show the current active task list, grouped into **In Progress** and **Open**.

---

## Configuration

Show the currently loaded configuration:

```console
shiori config show
```

Example output:

```text
version: 1
base_dir: C:\Users\you\Notes
```

Shiori searches for `.shiori` in this order:

1. the current working directory
2. the user's home directory

This allows you to use a project-specific configuration when needed while still keeping a general user-level configuration as a fallback.

### Using Shiori with Obsidian

Set `base_dir` to a directory inside your Obsidian vault:

```yaml
version: 1
base_dir: C:\Users\you\Documents\Obsidian\MyVault
```

Shiori will then maintain:

```text
NOTES.md
```

inside that directory.

Because the file is ordinary Markdown, Obsidian does not need a plugin or special integration.

---

## Interactive console

If you want to capture several notes without repeatedly invoking the executable:

```console
shiori console
```

Shiori opens an interactive prompt:

```text
shiori 🦊>
```

You can enter Shiori commands directly:

```text
shiori 🦊> add remember to fix the parser
shiori 🦊> config show
shiori 🦊> exit
```

Use either:

```text
exit
```

or:

```text
quit
```

to leave console mode.

---

## Debugging

Enable diagnostic output with:

```console
shiori --debug add something is behaving strangely
```

Debug output shows internal operations such as file lookup, heading detection, temporary-file handling, and backup replacement.

This is primarily intended for development and troubleshooting.

---

## Version information

```console
shiori version
```

prints Shiori's version together with build information such as compiler, platform, architecture, and detected C standard.

Example:

```text
shiori 0.1.0
compiler: clang 22.0.0
platform: windows x86_64
c standard: C23
```

---

## How Shiori stores your notes

Shiori deliberately avoids a proprietary storage format.

Notes use a chronological daily stream:

```markdown
# YYYY-MM-DD
* note
* another note
```

When adding to an existing day, Shiori:

1. opens `NOTES.md`,
2. writes a modified copy to `NOTES.md.tmp`,
3. locates today's Markdown heading,
4. inserts the new note,
5. backs up the original file,
6. replaces it with the updated file,
7. removes the backup after a successful replacement.

If replacement fails, Shiori attempts to restore the original file from its backup.

The Markdown file remains the source of truth.

Todos are stored separately in `TODOS.md` using Markdown task-list syntax plus Shiori metadata tags:

```markdown
---
version: 1
last_id: 4
---

* [ ] prepare release notes #work #shiori/id/1 #shiori/created/2026-08-12
* [/] verify the Windows build #shiori/id/2 #shiori/created/2026-08-12
* [x] update screenshots #shiori/id/3 #shiori/created/2026-08-11
```

Checkbox markers represent open (`[ ]`), in progress (`[/]`), and done (`[x]`). The front matter maintains the next stable ID. Status changes and other todo rewrites use the same temporary-file, backup, replacement, and restore strategy as notes.

---

## Philosophy

Shiori intentionally favors a few boring ideas:

**Local first.** Your thoughts should not require a network connection.

**Plain text.** The useful lifetime of a Markdown file is likely longer than the useful lifetime of most note-taking applications.

**Fast capture over organization.** Shiori is for getting the thought out of your head first. Structure can come later.

**Small software.** Shiori is intentionally compact and dependency-free. The goal is not to build a framework around writing one line into a Markdown file.

**Interoperability over lock-in.** If you stop using Shiori tomorrow, your notes are still Markdown.

---

## Roadmap

Shiori is intentionally being developed incrementally. Some ideas being explored include:

- complete Linux support
- versioned releases and easier installation
- configurable note ordering, including `append_on_top`
- richer note and todo handling beyond the current status, filtering, and editing workflow
- additional configuration commands
- improved interactive console parsing
- safer and more portable filesystem abstractions
- optional quality-of-life integrations while keeping Markdown as the source of truth

The project will stay focused on quick capture rather than turning into a full knowledge-management platform wearing a tiny CLI hat.

---

## Contributing

Contributions, bug reports, and ideas are welcome.

Please see [`CONTRIBUTING.md`](CONTRIBUTING.md) for development setup, contribution guidelines, and project conventions.

---

## License

Shiori is intended to be released under the [`MIT License`](LICENSE).

MIT is a good fit for a small utility like Shiori: it is permissive, widely understood, and allows people to use, modify, redistribute, or embed the project with minimal ceremony.

---

<div align="center">

**Write it down. Keep moving.**

🦊

</div>
