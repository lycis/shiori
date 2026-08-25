# Upgrading Shiori

This guide describes the user-visible upgrade steps between released versions
of Shiori. Read the section for the version you are installing before running
the new executable against an existing workspace.

Shiori is alpha software. Keep an independent backup of your workspace when
upgrading, even when a release provides an automatic migration.

## Upgrade from 0.2.0 to 0.3.0

Shiori 0.3.0 formalizes the `.shiori` configuration syntax and adds nested
maps. Normal 0.2.0 configuration files remain valid and the format version
stays at 1; no automatic rewrite is performed.

The existing top-level hook setting remains accepted:

```text
hook_after_command: hooks\after_command.bat
```

The canonical form for new and manually updated configurations is:

```text
hooks:
  after_command: hooks\after_command.bat
```

Storage filenames likewise move from the legacy top-level keys:

```text
notes_filename: NOTES.md
todo_filename: TODOS.md
```

to the canonical nested form:

```text
storage:
  notes: NOTES.md
  todo: TODOS.md
```

The legacy storage keys remain accepted with a deprecation warning. Do not
configure a legacy key and its corresponding nested key in the same file.

Do not configure both forms in one file. The legacy hook key remains accepted
with a deprecation warning. To disable the hook, remove the setting or use an
explicit empty string. A bare `hook_after_command:` is now interpreted as the
beginning of a map and is rejected when no child follows.

The new parser also rejects unknown keys, duplicate keys, tabs in indentation,
inconsistent nesting, and unsupported YAML constructs that older versions may
have silently ignored or interpreted as plain text. Errors identify the source
line and column. See the [configuration syntax reference](USER_GUIDE.md#configuration-file-syntax)
when adjusting a hand-edited file.

## Upgrade from 0.1.0 to 0.2.0

Shiori 0.2.0 introduces notes format version 1 and stable note IDs. Existing
`NOTES.md` files created by 0.1.0 are treated as format version 0 and require a
one-time migration.

The `.shiori` configuration remains at format version 1 and does not require a
migration. `TODOS.md` also requires no explicit migration for this upgrade.

### 1. Identify and back up the workspace

Make sure Shiori is using the intended `.shiori` configuration and `base_dir`.
The configured data directory contains `NOTES.md` and `TODOS.md`.

Close editors or synchronization tools that might modify these files while the
migration is running. Then copy at least these files to a location outside the
workspace:

- `.shiori`
- `NOTES.md`
- `TODOS.md`

For example, from PowerShell in a workspace whose data files are stored in the
current directory:

```powershell
New-Item -ItemType Directory -Path shiori-0.1.0-backup
Copy-Item .shiori, NOTES.md, TODOS.md -Destination shiori-0.1.0-backup
```

If `base_dir` points elsewhere, back up the files from that directory instead.
Do not rely solely on Shiori's automatic `.bak` file as your upgrade backup.

### 2. Install and verify Shiori 0.2.0

Replace the executable, then verify that the expected version is being run:

```console
shiori version
```

The first line should report:

```text
shiori 0.2.0
```

### 3. Run the notes migration

Run the migration where Shiori can load the configuration for the intended
workspace:

```console
shiori util migrate
```

The migration:

- adds version 1 front matter to `NOTES.md`
- assigns a stable ID to every note that does not already have one
- preserves existing IDs, note text, topics, dates, and ordering
- retains the previous complete file as `NOTES.md.bak`

Running the command again is safe. If the file is already current, it makes no
further changes.

### 4. Verify the result

Open `NOTES.md`. Its beginning should now resemble:

```markdown
---
version: 1
---

# 2026-08-23
* Example note <!-- shiori:id=20260823-0001 -->
```

Check that:

- all expected date sections and notes are present
- note text and topic tags are unchanged
- every note has a `<!-- shiori:id=... -->` comment
- commands such as `shiori today` and `shiori note show <id>` work as expected

Compare the migrated file with `NOTES.md.bak` or your independent backup. Keep
the backup until you are satisfied that the migration completed correctly.

The ID comments are part of Shiori's file format. They are hidden by normal
Markdown rendering, but should not be removed if notes are to remain addressable
by `note show`, `note retopic`, and `note remove`.

## If migration fails

Shiori refuses to rewrite a notes file when its structure is malformed or
contains content the full-file rewriter does not understand. This avoids
silently losing manually added Markdown.

On failure:

1. Read the reported error and do not repeatedly edit or migrate the file.
2. Confirm that the original `NOTES.md` is unchanged.
3. Check for `NOTES.md.bak`; if present, preserve it before doing anything else.
4. Compare `NOTES.md`, `NOTES.md.bak`, and the independent backup.
5. Correct the malformed or unsupported structure in a copy, then retry.

Shiori-managed notes files support version front matter, `# YYYY-MM-DD` date
headings, and note lines beginning with `* `. Move other Markdown to a separate
file before retrying a full-file migration.

If Shiori reports that replacement failed and that restoration also failed,
stop using the workspace. Recover `NOTES.md` from `NOTES.md.bak` or from the
independent backup before running another Shiori command.

## Other compatibility changes in 0.2.0

- Absolute dates must use exact `YYYY-MM-DD` syntax and represent a real
  calendar date. `today`, `yesterday`, and `tomorrow` remain supported.
- Redirected output is now plain text without ANSI color sequences. Color can
  also be disabled using `--no-color`, `color: false`, or `NO_COLOR`.
- Note and todo rewrites use temporary files and guarded replacement. Notes
  retain the previous complete file as `NOTES.md.bak`; successful todo rewrites
  remove their transient backup.
- Hooks remain Windows-only. Relative `base_dir` values are now resolved
  correctly.

For the complete list of changes, see the [changelog](../CHANGELOG.md). For the
current commands and storage formats, see the [user guide](USER_GUIDE.md).

---

Return to the [Shiori README](../README.md).
