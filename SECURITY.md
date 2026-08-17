Security Policy

Supported Versions

Shiori is currently in early development. Security fixes are provided for the latest released version only.

Version| Supported
Latest release| ✅
Older releases| ❌

Because Shiori is still evolving rapidly, users are encouraged to update to the latest available version before reporting a security issue.

Reporting a Vulnerability

If you discover a security vulnerability in Shiori, please report it using GitHub's private vulnerability reporting feature.

Do not open a public issue for security vulnerabilities.

When reporting a vulnerability, please include as much of the following information as possible:

- A description of the vulnerability
- Steps required to reproduce it
- The affected Shiori version
- The operating system and environment where the issue was observed
- The potential security impact
- Any suggested mitigation or fix, if known

Please avoid publicly disclosing the vulnerability until it has been investigated and, where necessary, a fix has been released.

What to Expect

Reported vulnerabilities will be reviewed to determine whether they represent a security issue and which versions are affected.

If a report is accepted, the issue will be investigated and a fix will be prepared where appropriate. Disclosure can then be coordinated through GitHub's security advisory workflow.

Reports may be declined if the behavior does not represent a security vulnerability or cannot reasonably be reproduced.

Scope

Security issues may include, but are not limited to:

- Arbitrary file access or modification outside Shiori's configured data directory
- Path traversal vulnerabilities
- Memory safety issues that could result in unintended code execution
- Unsafe parsing of Shiori configuration or Markdown files
- Command or argument handling that could result in unintended command execution
- Vulnerabilities that could cause Shiori to expose or overwrite user data

Normal application bugs, usability issues, crashes without security impact, and feature requests should be reported through the project's regular issue tracker.

Security Considerations

Shiori operates directly on local files and is intended to work with user-controlled Markdown content.

Users should avoid running Shiori with elevated privileges unless explicitly necessary. Shiori should only be used with configuration and data files from trusted sources.

As the project evolves, this policy and the supported-version policy may be updated accordingly.
