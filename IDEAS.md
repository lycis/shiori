# Ideas

Ideas and possible improvements for future Shiori versions.

## Storage & Configuration

* allow configuration of filenames for notes and todos instead of `NOTES.md` and `TODOS.md`
* allow a "daily notes mode" where notes are stored in one file per day
* `append_on_top` configuration switch
  * add new day blocks to the top of the notes file
  * optionally add new notes to the top of an existing day block
* consider configurable date formats for display while keeping storage format stable

## Notes

* stable IDs for notes
  * would enable reliable editing, removal, and direct lookup
* `note show <id>` for a detailed view of a note
* edit existing notes
* remove existing notes
* generated topic/tag views for Obsidian
  * derived from the master files, never a second source of truth

## Todos

* additional statuses
  * cancelled
  * deferred
* priorities
* track start date
* track completion date
* comments / additional details
  * possibly represented as Markdown sub-bullets
* `todo show <id>` for a detailed overview of a specific todo
* filtering by due date
  * overdue
  * due today
  * due this week
  * no due date
* possibly support recurring todos

## Topics & Tags

* list tags and usage counts
* autocomplete known topics
* autocomplete known tags
* consider hierarchical topic/tag navigation
* allow filtering by combinations of topic and tags

## Interactive Console & Capture

* command history with Up/Down
* proper cursor-aware insertion and deletion
* horizontal scrolling for input longer than the terminal width
* richer completion behavior
  * highlight selected completion
  * navigate suggestions with Up/Down
  * potentially complete command arguments as well as commands
* Ctrl+C handling that always restores terminal state cleanly

## Search & Navigation

* full-text search across notes and todos
* date-range filtering
* search by topic, tag, date, or todo state in combinations
* possibly introduce a unified `search` command

## Hooks & Automation

* additional hook points
  * before command
  * after note capture
  * after todo change
* expose command result to hooks
* document hook environment variables as a stable interface
* example hooks for:
  * Git sync
  * backups
  * external processing

## Platform Support

* Linux support
* replace or abstract Windows-specific secure CRT dependencies where necessary
* verify interactive terminal behavior across Windows Terminal, PowerShell, cmd, and POSIX terminals

## Quality of Life

* configurable output colors
* optional plain/no-color output
* improve handling of very long interactive input lines
* quoted arguments in console and capture mode
  * e.g. `todo add "something with spaces"`

## Markdown Support

* Recognize markdownlinks in text and highlight them
* Interpret **bold** and *italic* in texts