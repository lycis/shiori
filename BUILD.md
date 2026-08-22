# Building Shiori

Shiori is a C23 project built with Clang and GNU Make. Windows x64 is the
supported build platform. The required LLVM version is pinned in
`.llvm-version`; `make` refuses to build when `clang`, `clang-format`, or
`clang-tidy` does not report that version.

## Prerequisites

Install the following tools and make sure they are available on `PATH`:

- LLVM/Clang matching `.llvm-version` (currently 22.1.8), including
  `clang`, `clang-format`, and `clang-tidy`
- Visual Studio Build Tools with the Desktop development with C++ workload
  (MSVC headers/libraries and a Windows SDK)
- GNU Make
- Python 3 and the packages in `requirements-test.txt` to run integration
  tests

Check the native toolchain with:

```console
make toolchain-check
```

On Windows, run the build from an x64 Native Tools/Developer Command Prompt,
or first load the equivalent Visual Studio developer environment. LLVM's
Windows target uses the MSVC and Windows SDK headers and libraries. The
`toolchain-check` target compiles and links a small probe so a normal shell
without that environment stops immediately with setup instructions instead of
failing later on a missing standard header or library.

Install the Python test dependency with:

```console
python -m pip install -r requirements-test.txt
```

The integration suite currently pins Robot Framework 7.4.2.

## Build targets

Run these commands from the repository root.

```console
make
```

`make` is the same as `make debug`. It compiles every `src/*.c` file with
C23, strict warnings, no optimization, and DWARF debug information. Objects
and dependency files go to `build/debug/`; the linked executable is copied to
`shiori.exe` in the repository root.

```console
make release
```

The release build uses `-O2`. On Windows it statically links the MSVC C
runtime with `-fms-runtime-lib=static`, so the executable does not require a
separate Visual C++ Redistributable. Intermediate files and the linked binary
are written under `build/release/`, and the result is copied to `shiori.exe`.

```console
make sanitize
```

The sanitizer build uses AddressSanitizer on Windows and AddressSanitizer plus
UndefinedBehaviorSanitizer on other platforms. Its files are placed in
`build/sanitize/`. On Windows, the Clang AddressSanitizer runtime DLL is copied
beside the sanitizer executable.

Other useful targets are:

| Target | Purpose |
|---|---|
| `make run` | Build the debug executable and run it |
| `make clean` | Remove `build/`, the root executable, and its PDB if present |
| `make format` | Reformat all `src/*.c` and `src/*.h` files in place |
| `make format-check` | Check formatting without changing files |
| `make tidy` | Run `clang-tidy`, treating every warning as an error |

The Makefile generates `.d` dependency files alongside object files, so
changes to included headers trigger the necessary recompilation.

## Tests and project checks

Run the C unit tests and Robot Framework integration tests with:

```console
make test
```

The individual targets are `make test-unit` and `make test-integration`.
Unit-test executables are stored in `build/unit/`; integration-test reports
are stored in `build/test-results/`.

Run the full local validation used for development with:

```console
make check
```

This runs the formatting check, static analysis, unit tests, and integration
tests. To build the sanitizer executable and run the integration suite against
it, use:

```console
make check-sanitize
```

If `python` is not the correct Python command on your system, override it for
Make, for example:

```console
make test PYTHON=py
```

## Continuous integration and releases

`.github/workflows/build.yml` runs on Windows for pushes to `main`, pull
requests, and manual dispatches. It downloads the LLVM version named in
`.llvm-version`, installs the Python test dependency, and uses the same
Makefile targets as local development:

```console
make toolchain-check
make check
make check-sanitize
make release
```

This covers formatting, static analysis, C unit tests, integration tests, and
the AddressSanitizer integration run. CI then verifies that the release imports
no dynamic C/C++ runtime DLL and uploads `shiori.exe` as the
`shiori-windows-x64` artifact.

`.github/workflows/release-build.yml` runs for tags shaped like `X.Y.Z`. It
uses `make release`, performs the same runtime check, then packages
`shiori.exe`, `LICENSE`, and `README.md` as
`shiori-X.Y.Z-windows-x64.zip` and generates a SHA-256 checksum file.

## Platform note

The Makefile contains non-Windows paths and enables AddressSanitizer plus
UndefinedBehaviorSanitizer there, but the project is Windows-first and CI only
validates the application build on Windows. Treat builds on other operating
systems as experimental.
