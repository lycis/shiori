# Shiori User Guide

Shiori is a local-first command-line scratchpad and task tracker. It stores notes and todos as ordinary Markdown files in a directory you control.

This guide covers installation, configuration, every current workflow, and the on-disk formats. For the project overview and build status, see the [README](../README.md).

## Contents

- [Installation](#installation)
- [Initialization and configuration](#initialization-and-configuration)
- [Hooks and automation](#hooks-and-automation)
- [Command overview](#command-overview)
- [Managing notes](#managing-notes)
- [Capture sessions](#capture-sessions)
- [Organizing notes with topics](#organizing-notes-with-topics)
- [Finding notes and todos by tag](#finding-notes-and-todos-by-tag)
- [Managing todos](#managing-todos)
- [Daily dashboard](#daily-dashboard)
- [PowerShell completion](#powershell-completion)
- [Interactive console](#interactive-console)
- [Debugging](#debugging)
- [Version information](#version-information)
- [Migrating an existing notes file](#migrating-an-existing-notes-file)
- [Data storage and safety](#data-storage-and-safety)
- [Using Shiori with Obsidian](#using-shiori-with-obsidian)

## Installation

### Download the standalone executable

Every successful GitHub Actions build publishes a `shiori-windows-x64` artifact containing `shiori.exe`. Download the artifact, extract it, and verify the executable:

```console
.\shiori.exe help
```

The release executable statically links the C runtime. It does not require a separate Visual C++ Redistributable or third-party DLLs; it only uses DLLs provided by Windows.

### Build from source

You need:

- a C23-capable compiler
- GNU Make
- currently, a Windows development environment with Clang

Build a debug executable:

```console
make
```

Build an optimized release executable:

```console
make release
```

Remove build output:

```console
make clean
```

### Put Shiori on your PATH

You can run `shiori.exe` directly from the project directory or copy it to a directory on your user `PATH`, such as:

```text
C:\Tools\shiori\
```

After adding that directory to `PATH`, this should work from any terminal:

```console
shiori help
```

## Initialization and configuration

Initialize Shiori in the current directory:

```console
shiori init
```

This creates a `.shiori` file similar to:

```yaml
# Initialized: ...
version: 1
base_dir: C:\path\to\your\notes
color: true
```

By default, `base_dir` is the directory where you ran `shiori init`. Shiori stores `NOTES.md` and `TODOS.md` there. The `color` setting defaults to `true` when it is omitted, so existing configurations keep colored interactive output.

To intentionally recreate an existing local configuration:

```console
shiori init --reinit
```

Show the loaded configuration:

```console
shiori config show
```

Shiori searches for `.shiori` in this order:

1. the current working directory
2. the user's home directory

A project-specific configuration therefore takes precedence over a user-level fallback.

## Hooks and automation

Shiori can run a local script after a command, making it possible to trigger backups, version-control workflows, notifications, or other personal automation.

Configure the current `after_command` hook in `.shiori` with a path relative to `base_dir`:

```yaml
version: 1
base_dir: C:\Users\you\Notes
hook_after_command: hooks\after_command.bat
```

The hook runs synchronously after recognized commands and receives command information through environment variables. Hook scripts are executable code, so only configure scripts you trust.

`shiori config show` displays the configured hook path under `hooks`, or `(not configured)` when it is disabled.

See the dedicated **[Shiori Hooks Guide](HOOKS.md)** for the complete lifecycle, environment variables, platform support, examples, failure behavior, and current limitations.

## Command overview

```text
shiori [options] <command> [options] [subcommand] ...
```

### Global options

| Option | Description |
|---|---|
| `--debug` | Show debug and plumbing output. |
| `--no-color` | Disable ANSI color sequences. |

### Color output

Shiori colors output when the destination is an interactive terminal and color is enabled in `.shiori`:

```yaml
color: true
```

Set `color: false` to disable ANSI color sequences for commands that use that configuration. You can also disable color for an individual invocation:

```console
shiori --no-color today
```

Shiori honors the [`NO_COLOR`](https://no-color.org/) convention. A present, non-empty `NO_COLOR` environment variable disables color; an empty value is ignored. Output redirected to a file or pipe is always emitted without ANSI color sequences. Standard output and standard error are detected independently.

The precedence from strongest to weakest is:

1. `--no-color` or a non-empty `NO_COLOR`
2. output redirection
3. `color: false` in `.shiori`
4. colored interactive output by default

No-color output retains Unicode symbols and layout; it removes ANSI styling only.

### Commands

| Command | Description |
|---|---|
| `init` | Initialize a new `.shiori` configuration. |
| `add` | Add a note to today's section (short form of `note add`). |
| `note` | Add, inspect, retopic, and remove notes by ID. |
| `capture` | Start an interactive session for capturing notes and todos. |
| `topic` | Show notes for a topic or list topic statistics. |
| `tag` | Find notes and todos containing all specified tags. |
| `todo` | Add, list, update, and remove todos. |
| `today` | Show notes and active todos in a daily dashboard. |
| `config` | Show the loaded configuration. |
| `console` | Start interactive console mode. |
| `util` | Run workspace migrations and generate shell completion. |
| `help` | Show command help. |
| `version` | Display version and build information. |

## Managing notes

Every new note receives a stable ID in `YYYYMMDD-NNNN` form. Shiori prints this ID after capture and displays it in the daily dashboard, topic views, and tag results. Use it with the note-management commands described below.

### Adding notes

Add a thought without opening an editor:

```console
shiori add remember to review the architecture diagram
```

`note add` is an equivalent, more explicit form:

```console
shiori note add remember to review the architecture diagram
```

If today's section already exists in `NOTES.md`, Shiori adds the note to that block. Otherwise it creates the section:

```markdown
# 2026-08-22
* remember to review the architecture diagram <!-- shiori:id=20260822-0001 -->
* investigate SQLite later, but probably do not need it <!-- shiori:id=20260822-0002 -->

# 2026-08-21
* rename the project <!-- shiori:id=20260821-0001 -->
```

Notes are stored as Markdown bullet points using `*`; the HTML comment holds Shiori's stable ID without affecting normal Markdown rendering.

Use `shiori add --help` for the built-in reference.

### Inspecting a note

Show a note's creation date, topic, tags, and text:

```console
shiori note show 20260822-0001
```

### Changing a note's topic

Assign a different topic while preserving the note's ID, text, tags, and date:

```console
shiori note retopic 20260822-0001 architecture
```

Remove its topic with the special value `none`:

```console
shiori note retopic 20260822-0001 none
```

### Removing a note

Remove a note from the active log by ID:

```console
shiori note remove 20260822-0001
```

Run `shiori note help` for the complete built-in note reference.

## Capture sessions

Use `capture` when you want to collect several thoughts without invoking Shiori separately for every item:

```console
shiori capture
```

Shiori opens a focused prompt. Enter an ordinary line to add it as a note, or begin the line with `!` to add it as a todo:

```text
~> summarize the customer interview
~> ! send the follow-up email
~> ! --due tomorrow prepare the proposal
~> /done
```

The session writes each item immediately. Todos accept the same `--due` or `-d` date option as `todo add`:

```text
~> ! -d 2026-08-20 publish the release notes
~> ! --due today verify the build
```

Due-date values may be `YYYY-MM-DD`, `today`, `yesterday`, or `tomorrow`.

### Capture notes under a topic

Assign every note in the session to one topic with `--topic` or `-t`:

```console
shiori capture --topic Rail4Climate
shiori capture -t Rail4Climate
```

The prompt shows the active topic:

```text
~Rail4Climate> discuss pilot scope
~Rail4Climate> collect stakeholder feedback
~Rail4Climate> ! --due tomorrow schedule the workshop
~Rail4Climate> /done
```

The session topic applies to captured notes. Todos retain their entered text and optional due date; they are not assigned the note topic metadata.

End the session with any of:

```text
/done
/exit
/quit
```

Press Escape to cancel the capture session cleanly. Press Ctrl+C to interrupt Shiori immediately; the process restores the original terminal mode and exits with status `130`.

Blank lines are ignored, and an empty `!` todo is rejected without ending the session. Use `shiori capture --help` for the built-in reference.

## Organizing notes with topics

Assign one topic to a note with `--topic` or its `-t` shorthand:

```console
shiori add --topic Rail4Climate discuss pilot scope
shiori add -t Rail4Climate review the proposal
```

Shiori keeps the topic in the note as a metadata tag while showing the readable topic name in terminal views:

```markdown
# 2026-08-22
* discuss pilot scope #shiori/topic/Rail4Climate <!-- shiori:id=20260822-0001 -->
* review the proposal #shiori/topic/Rail4Climate <!-- shiori:id=20260822-0002 -->
```

Show all notes assigned to a topic, grouped under their daily headings:

```console
shiori topic Rail4Climate
```

List every topic with its number of notes:

```console
shiori topic --list
shiori topic -l
```

The daily dashboard displays a note's topic beside its text. Topic names are currently single values without spaces.

Use `shiori topic --help` for the built-in reference.

## Finding notes and todos by tag

Tags are ordinary `#name` tokens in note or todo text. Unlike a topic, which is structured note metadata assigned with `--topic`, tags can be added freely and are shared by notes and todos.

Add tags as part of the captured text. Quote them in PowerShell because an unquoted `#` begins a comment:

```console
shiori add record the architecture decision "#decision" "#backend"
shiori todo add follow up with the team "#backend"
```

Search both `NOTES.md` and `TODOS.md` by writing tag names without the leading `#`:

```console
shiori tag backend
```

The result groups matching notes under their dates and then lists matching todos with status, ID, and due date when present. Todos of every status are included.

Specify multiple tags to require all of them:

```console
shiori tag decision backend
```

Matching uses complete tags, so searching for `work` does not match `#workshop`.

Use `shiori tag --help` for the built-in reference. The `todo list --tag` filters remain useful when you only want todos and want to combine tags with todo status filters.

## Managing todos

Todos live in `TODOS.md`. Add one with:

```console
shiori todo add prepare release notes "#work"
```

Each todo receives a stable numeric ID, a creation date, and an initial status of open. Shiori prints the assigned ID after creation; use it for status changes, rewrites, and removal.

Quote tags when using PowerShell because an unquoted `#` starts a comment.

### Due dates

Add an optional due date with `--due` or `-d`. The date may be an ISO date or one of `today`, `tomorrow`, and `yesterday`:

```console
shiori todo add --due 2026-08-20 prepare release notes
shiori todo add -d tomorrow verify the Windows artifact
```

`shiori todo list` shows the creation date for every task and an additional due date for tasks that have one.

### Change status

Use the task ID to move it through its lifecycle:

```console
shiori todo start 1
shiori todo done 1
shiori todo reopen 1
```

- `start` moves a task to in progress.
- `done` marks a task as completed.
- `reopen` returns a task to open.

### List and filter

By default, the list contains open and in-progress todos:

```console
shiori todo list
```

Filter by status:

```console
shiori todo list --open
shiori todo list --in-progress
shiori todo list --done
shiori todo list --all
```

Explicit status switches replace the default selection. Multiple status switches combine.

Filter by tags contained in the todo text:

```console
shiori todo list --tag work
shiori todo list --tag work --tag urgent
shiori todo list --done --tag work
```

`--tag` may be repeated. A todo must contain every selected tag to match.

Filter by due date:

```console
shiori todo list --overdue
shiori todo list --due-today
shiori todo list --due-this-week
shiori todo list --no-due-date
```

`--overdue` means due before today, while `--due-today` matches today exactly.
`--due-this-week` uses a fixed Monday-through-Sunday calendar week, regardless of
the system locale. The four date switches are mutually exclusive. A selected date
filter is combined with status and every `--tag` filter using AND semantics; for
example, `todo list --done --overdue --tag work` shows completed, overdue todos
that contain `#work`. Without a date switch, todos are not filtered by due date.

### Rewrite and remove

Change a todo's text while preserving its ID, status, creation date, and due date:

```console
shiori todo rewrite 1 prepare final release notes
```

Set or change only the due date, change text and due date together, or remove the due date with `none`:

```console
shiori todo rewrite 1 --due tomorrow
shiori todo rewrite 1 prepare final release notes --due 2026-08-20
shiori todo rewrite 1 --due none
```

Permanently remove one todo:

```console
shiori todo remove 1
```

Remove all completed todos by first previewing the number affected and then confirming:

```console
shiori todo prune
shiori todo prune --force
```

Run `shiori todo --help` or `shiori todo list --help` for the built-in command reference.

## Daily dashboard

Show today's notes and current active todos in a styled terminal dashboard:

```console
shiori today
```

Select another day with an ISO date or relative selector:

```console
shiori today --date 2026-08-12
shiori today --date yesterday
shiori today --date tomorrow
shiori today --date today
```

The **Notes** section follows the selected date and displays assigned topics. Active todos are grouped relative to the selected dashboard date:

- **Overdue** contains unfinished todos due before the selected date.
- **Due Today** contains unfinished todos due on the selected date, or an “all clear” message when empty.
- **In Progress** and **Open** contain the remaining active todos.

Completed todos are not shown in the dashboard.

Use `shiori today --help` for the built-in reference.

## PowerShell completion

Shiori can generate native PowerShell completion from its command registry, including nested `note`, `todo`, and `util` subcommands.

Load completion for the current PowerShell session:

```powershell
shiori util completion powershell | Out-String | Invoke-Expression
```

Or write the generated script to a file that you can load from your PowerShell profile:

```powershell
shiori util completion powershell > shiori-completion.ps1
. .\shiori-completion.ps1
```

The alias `pwsh` is also accepted as the shell argument. The generated script is written to standard output.

## Interactive console

Keep Shiori open while capturing several thoughts:

```console
shiori console
```

The interactive prompt accepts Shiori commands directly:

```text
shiori 🦊> add remember to fix the parser
shiori 🦊> config show
shiori 🦊> exit
```

Enter `exit` or `quit` to leave console mode.

Press Escape to cancel the console session cleanly. Press Ctrl+C to interrupt Shiori immediately; the process restores the original terminal mode and exits with status `130`.

As you type a command, Shiori displays matching suggestions in color. Press Tab to accept a single match; when several commands match, Tab expands the input to their longest shared prefix. Console completion includes Shiori commands plus `exit` and `quit`.

Capture mode uses the same UTF-8-aware input renderer and suggests its slash commands (`/done`, `/exit`, and `/quit`).

Input remains on one terminal row. When a command grows wider than the available space, Shiori scrolls the line horizontally to keep the editing cursor visible. `Home` reveals the beginning, `End` reveals the end, and the arrow keys move through the complete input rather than only the visible portion. Resizing the terminal recalculates the visible portion without changing the command.

Interactive input accepts at most 1,023 UTF-8 bytes. Once that limit is reached, additional characters are ignored while the existing command remains available for editing or submission.

## Debugging

Enable diagnostic output with the global `--debug` option:

```console
shiori --debug add something is behaving strangely
```

Debug output includes internal operations such as file lookup, heading detection, temporary-file handling, and backup replacement. It is intended for development and troubleshooting.

## Version information

```console
shiori version
```

The output includes Shiori's version, compiler, platform, architecture, and detected C standard:

```text
shiori 0.2.0
compiler: clang 22.1.8
platform: windows x86_64
c standard: C23
```

## Migrating an existing notes file

The current `NOTES.md` format is version 1. Older files without front matter or stable note IDs are treated as version 0 and produce a warning when read.

Upgrade the configured workspace with:

```console
shiori util migrate
```

Migration adds version front matter and assigns IDs to notes that do not already have one. Existing IDs, topics, text, dates, and ordering are preserved. The migration is idempotent, so running it again on an up-to-date file makes no further changes.

Review the retained `NOTES.md.bak` recovery copy after migration before removing it.

## Data storage and safety

Shiori avoids proprietary storage. Markdown remains the source of truth.

### Notes

`NOTES.md` uses versioned front matter followed by chronological daily sections:

```markdown
---
version: 1
---

# 2026-08-22
* note <!-- shiori:id=20260822-0001 -->
* another note #shiori/topic/example <!-- shiori:id=20260822-0002 -->
```

The note ID combines the creation date with a per-day sequence. Topic metadata and IDs remain ordinary Markdown text, while the ID comment stays hidden in rendered output.

When changing `NOTES.md`, Shiori:

1. opens `NOTES.md`
2. writes a modified copy to `NOTES.md.tmp`
3. creates `NOTES.md.bak` from the previous complete file
4. replaces the source with the updated file
5. reads the result back and validates the expected note count

If replacement or validation fails, Shiori attempts to restore the original. After a successful change, `NOTES.md.bak` is deliberately retained as a recovery copy and replaced by the previous complete file on the next change.

Shiori-managed note files accept front matter, `# YYYY-MM-DD` headings, and `* ` note items. Operations that rewrite the complete file, such as `note retopic`, `note remove`, and migration, refuse unexpected content rather than silently discarding manually added Markdown.

### Todos

`TODOS.md` uses Markdown task-list syntax plus Shiori metadata tags:

```markdown
---
version: 1
last_id: 4
---

* [ ] prepare release notes #work #shiori/id/1 #shiori/created/2026-08-12 #shiori/due/2026-08-20
* [/] verify the Windows build #shiori/id/2 #shiori/created/2026-08-12 #shiori/due/2026-08-13
* [x] update screenshots #shiori/id/3 #shiori/created/2026-08-11
```

Checkbox markers represent open (`[ ]`), in progress (`[/]`), and done (`[x]`). The optional `#shiori/due/YYYY-MM-DD` tag stores a due date. The front matter maintains the next stable ID. Todo rewrites also use temporary and backup files, but unlike note rewrites they remove the backup after a successful replacement.

## Using Shiori with Obsidian

Set `base_dir` to a directory inside your Obsidian vault:

```yaml
version: 1
base_dir: C:\Users\you\Documents\Obsidian\MyVault
```

Shiori maintains `NOTES.md` and `TODOS.md` inside that directory. Both are ordinary Markdown, so Obsidian requires no plugin or special integration.

---

Return to the [Shiori README](../README.md).
