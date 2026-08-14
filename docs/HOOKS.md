# Shiori Hooks Guide

Shiori hooks connect commands to local automation. A hook is an executable or script that Shiori starts at a defined point in the command lifecycle.

This is the technical reference for every supported hook. Shiori currently provides one hook, `after_command`; future hook points will be documented here as they are added.

For general usage, see the [User Guide](USER_GUIDE.md). For the project overview, see the [README](../README.md).

## Contents

- [Security](#security)
- [Configuration](#configuration)
- [Hook execution model](#hook-execution-model)
- [`after_command`](#after_command)
- [Environment variables](#environment-variables)
- [Writing a hook](#writing-a-hook)
- [Included examples](#included-examples)
- [Troubleshooting](#troubleshooting)
- [Current limitations](#current-limitations)

## Security

Hooks execute local code with the same operating-system permissions and environment as Shiori. A hook can read, modify, or delete files and can start other programs or access the network.

Only configure hook scripts that you trust and have reviewed. Pay particular attention to scripts that stage files, create commits, push to remotes, upload data, or interpolate environment variables into commands.

## Configuration

Configure hooks in the same `.shiori` file as `base_dir`:

```yaml
version: 1
base_dir: C:\Users\you\Notes
hook_after_command: hooks\after_command.bat
```

`hook_after_command` is optional. An empty or absent value disables it.

The hook path is resolved relative to `base_dir`, not relative to the directory containing `.shiori` or the shell's current directory. With the configuration above, Shiori runs:

```text
C:\Users\you\Notes\hooks\after_command.bat
```

The hook process also uses `base_dir` as its working directory. This makes paths such as `NOTES.md` and `TODOS.md` directly accessible from the script.

Check the loaded value with:

```console
shiori config show
```

The output includes either the configured relative path or `(not configured)`:

```text
version: 1
base_dir: C:\Users\you\Notes

hooks:
  after_command: hooks\after_command.bat
```

## Hook execution model

Hooks are synchronous: Shiori starts the process and waits for it to exit before returning control to the terminal. A slow hook therefore makes the originating command appear slower.

For recognized commands that load configuration, `after_command` runs after the command handler returns. It is still invoked when that handler reports an error. It is not invoked for `init`, `help`, or `version`, because those commands run before configuration is loaded. An unknown command does not trigger it.

Interactive modes have slightly different granularity:

- `capture` triggers `after_command` once when the capture session ends, not once per captured note or todo.
- Commands entered inside `console` trigger their own hooks. Exiting the console then triggers another hook for the outer `console` command.

Hook execution does not change the originating command's result. A nonzero hook exit code produces a warning, while Shiori preserves and returns the command handler's result.

## `after_command`

Configuration key:

```yaml
hook_after_command: path\relative\to\base_dir.bat
```

Timing: after a recognized, configuration-dependent command completes.

Current command examples include:

- `add`
- `capture`
- `config`
- `console`
- `today`
- `todo`
- `topic`

Before starting the hook, Shiori exports the command name, arguments, and Shiori version as environment variables.

## Environment variables

| Variable | Meaning | Example |
|---|---|---|
| `SHIORI_VERSION` | Shiori application version. | `0.1.0` |
| `SHIORI_COMMAND` | Top-level command name. | `todo` |
| `SHIORI_COMMAND_ARGS` | Command arguments joined with single spaces. | `add --due tomorrow prepare release notes` |

`SHIORI_COMMAND_ARGS` is a flattened display string. Original quoting and argument boundaries are not preserved, so do not treat it as a safely escaped command line or execute it directly.

The hook also inherits the rest of Shiori's process environment.

## Writing a hook

### Minimal Windows batch hook

Create `hooks\after_command.bat` under `base_dir`:

```bat
@echo off
echo [%date% %time%] %SHIORI_COMMAND% %SHIORI_COMMAND_ARGS%>>shiori-hook.log
exit /b 0
```

Then add it to `.shiori`:

```yaml
hook_after_command: hooks\after_command.bat
```

Run a command and inspect the resulting log:

```console
shiori add test the hook
type shiori-hook.log
```

### Branch on the command

```bat
@echo off
if /I "%SHIORI_COMMAND%"=="add" (
    echo A note command completed.
)
exit /b 0
```

Always return a meaningful exit code. Shiori treats zero as success and logs a warning for any other value.

## Included examples

The repository contains two Windows batch examples in [`example-hooks`](../example-hooks):

- [`after_command_hello.bat`](../example-hooks/after_command_hello.bat) appends a line to `hook.txt`.
- [`push_to_git.bat`](../example-hooks/push_to_git.bat) stages changes, commits them using the Shiori command context, and pushes them.

The Git example is intentionally powerful: it runs `git add .` and `git push`. Review and adapt it before use, and only point it at a repository where automatically staging and pushing every change is acceptable.

To use an included example when this repository is also your `base_dir`:

```yaml
hook_after_command: example-hooks\after_command_hello.bat
```

## Troubleshooting

Run Shiori with `--debug` to see hook activation and process errors:

```console
shiori --debug add test the hook
```

If the hook does not run:

1. Run `shiori config show` and verify the loaded hook value.
2. Resolve the configured path under `base_dir` and confirm the file exists.
3. Run the script manually with `base_dir` as the working directory.
4. Check whether Windows allows the process or script to execute.
5. Inspect Shiori's warning for a process-start error or nonzero exit code.

## Current limitations

- Only `after_command` is supported.
- Hook execution is currently Windows-first; `.bat` and `.cmd` files are launched through `cmd.exe`.
- Hooks run synchronously, with no timeout or background mode.
- Hook paths are relative to `base_dir`.
- Hook standard output and standard error are inherited rather than captured.
- `SHIORI_COMMAND_ARGS` does not preserve original quoting or argument boundaries.
- Hook failures warn but do not override the Shiori command's result.

---

Return to the [User Guide](USER_GUIDE.md) or [README](../README.md).
