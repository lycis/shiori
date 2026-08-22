# Coding Style

This document defines the coding conventions for Shiori's C source code. Other project files, such as CSS, Markdown,
Robot Framework tests, and Makefiles, are outside its scope.

## Formatting

- Format the entire C codebase with the project's enforced `clang-format` configuration.
- Use four spaces for indentation and a maximum line length of 120 characters.
- Keep opening braces on the same line.
- Do not add a space after control-flow keywords: write `if(condition)`, `for(...)`, and `while(...)`.
- Always use braces for control-flow bodies, including one-line `if`, `else`, `for`, `while`, and `do` bodies.
- Write pointers as `char *value`.
- Place declarations near their first use.
- When a declaration or call must span multiple lines, put each parameter or argument on its own line and place the
  closing `);` on its own line.

```c
static int create_note(
    const char *text,
    size_t text_size,
    struct note *result
) {
    if(text == NULL || result == NULL) {
        return R_ERROR;
    }

    return write_note(
        text,
        text_size,
        result
    );
}
```

## Design and naming

- Prefer clear names and small, expressive steps over explanatory comments.
- Comment only intent, constraints, or behavior that is not evident from the code.
- Use explicit struct names such as `struct note`; avoid typedefs that hide the underlying type.
- Plain enum values such as `OPEN` are acceptable when their meaning is clear from context.
- Declare internal functions `static`. Export functions only when another module uses them.
- Use conventional, non-reserved include guards such as `SHIORI_COMMON_H`.
- Include standard-library headers first, followed by project headers.

## Control flow and errors

- Use early returns for invalid input and failures so the successful path remains linear.
- Avoid `goto`; use it only as a justified exception.
- Use `R_OK` and `R_ERROR` by default. Introduce more descriptive results when callers need to distinguish failures.
- Validate input at trust boundaries or when misuse is plausible. Redundant null checks may be omitted in tightly
  controlled call paths.

## Portability and buffers

- Prefer portable C. Platform-specific behavior belongs exclusively in the platform module.
- Prefer fixed-size buffers.
- Treat insufficient buffer capacity as an error; never silently truncate data.

## Verification

- CI treats compiler warnings as errors and runs `clang-tidy`.
- CI runs AddressSanitizer and UndefinedBehaviorSanitizer where supported.
- Behavior changes should include tests.
