# Changelog

Notable changes to Shiori are documented in this file.

## [0.3.0] - Unreleased

### Added

- Cancelled (`[-]`) and deferred (`[>]`) todo statuses, including lifecycle
  commands, list filters, dashboard presentation, help, and completion.

## [0.2.0] - 2026-08-23

Shiori 0.2.0 expands the original scratchpad into a more complete local notes
and todo workflow. This release adds stable note IDs, note management, richer
todo views, interactive completion, safer file replacement, and substantially
improved documentation and tests.

Shiori remains alpha software and Windows-first.

### Upgrade from 0.1.0

Back up the directory containing `.shiori`, `NOTES.md`, and `TODOS.md` before
upgrading.

Existing `NOTES.md` files from 0.1.0 use notes format version 0. After installing
0.2.0, run this once from a directory where Shiori can load the intended
configuration:

```console
shiori util migrate
```

The migration adds version 1 front matter and assigns a stable ID to every note
that does not already have one. Existing IDs, note text, topics, dates, and
ordering are preserved. The command is idempotent and may safely be run again.

After migration:

- Inspect `NOTES.md` and the retained pre-migration recovery copy
  `NOTES.md.bak` before deleting the backup.
- Do not remove the `<!-- shiori:id=... -->` comments if you want note commands
  such as `show`, `retopic`, and `remove` to keep addressing notes reliably.
- No `.shiori` configuration migration is required; its format remains version
  1.
- No explicit `TODOS.md` migration command is required.

### Safety and compatibility notes

- Commands that rewrite `NOTES.md` now write a temporary file, retain the
  previous complete file as `NOTES.md.bak`, install the new file, and validate
  the result. If replacement or validation fails, Shiori attempts to restore
  the backup.
- Full note rewrites, including migration, `note retopic`, and `note remove`,
  reject unexpected content instead of silently discarding Markdown they do
  not understand. If migration fails, the source file should remain unchanged;
  correct the reported malformed or unsupported content and try again.
- Todo rewrites also use temporary and backup files. Their transient backup is
  removed after a successful replacement.
- Absolute dates now require the exact `YYYY-MM-DD` form and must be real
  calendar dates. Inputs previously accepted accidentally, such as
  `2030-02-31`, `2030-2-03`, or a date with trailing characters, are rejected.
  The relative values `today`, `yesterday`, and `tomorrow` remain supported.
- `todo prune` only removes completed todos when explicitly invoked with
  `--force`; without it, Shiori reports what would be removed.
- Redirected output now disables ANSI color automatically. Scripts that parsed
  colored redirected output should expect plain text. Color can also be
  disabled with `--no-color`, `color: false`, or the `NO_COLOR` environment
  variable.
- Hook execution remains Windows-only. Hooks receive `SHIORI_VERSION=0.2.0`,
  and relative `base_dir` configurations are now resolved correctly.

### Added

- Stable IDs for notes and IDs in note creation, `today`, `topic`, and `tag`
  output.
- `note add`, `note show`, `note retopic`, and `note remove` commands.
- `shiori util migrate` for upgrading legacy notes to format version 1.
- PowerShell completion generation and interactive command/subcommand
  completion.
- Interactive history navigation, cursor movement, insertion, Delete, Home,
  End, scrolling for long input, and safer long Unicode input handling.
- `todo show` with status-aware color and icons.
- Due-date filters for todo lists and strict calendar-date validation.
- Tag statistics with `tag --list`.
- Global color policy, `.shiori` color setting, `--no-color`, `NO_COLOR`
  support, and automatic plain output when redirected.
- Detailed version/build information from `shiori version`.
- User, build, hook, contribution, coding-style, and security documentation.
- Unit and integration coverage for CLI behavior, migrations, storage safety,
  notes, todos, colors, queries, and terminal lifecycle.

### Fixed

- Note operations no longer target or delete the wrong `NOTES.md` when run
  outside the configured `base_dir`.
- Todo operations consistently use the configured data directory.
- Notes metadata is initialized on first write and empty metadata files are
  handled correctly.
- UTF-8 paths use platform-specific file wrappers.
- Terminal state is restored on interactive exits and retained correctly
  between input lines.
- Capture mode routes todo additions correctly.
- Nested subcommand routing and completion behavior.
- Topic filtering with `--topic`.
- Failed todo status changes now return an error.
- Relative hook base directories are resolved correctly.

## [0.1.0]

Initial release.

[0.2.0]: https://github.com/lycis/shiori/compare/0.1.0...0.2.0
[0.1.0]: https://github.com/lycis/shiori/releases/tag/0.1.0
