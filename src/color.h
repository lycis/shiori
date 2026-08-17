#ifndef _SHIORI_COLOR_H
#define _SHIORI_COLOR_H

#define ANSI_BOLD  "\x1b[1m"
#define ANSI_RESET "\x1b[0m"
#define ANSI_FG_RGB(r, g, b) "\x1b[38;2;" #r ";" #g ";" #b "m"

// Semantic colors
#define COLOR_SUCCESS   ANSI_FG_RGB(100, 210, 140)
#define COLOR_ERROR     ANSI_FG_RGB(255, 105, 120)
#define COLOR_WARNING   ANSI_FG_RGB(255, 190, 80)
#define COLOR_INFO      ANSI_FG_RGB(110, 190, 255)

#define COLOR_TOPIC     ANSI_FG_RGB(180, 140, 255)

#define COLOR_OVERDUE   COLOR_ERROR
#define COLOR_DUE_TODAY COLOR_WARNING

// Interactive terminal / completion

#define COLOR_COMPLETION_MATCH     COLOR_SUCCESS
#define COLOR_COMPLETION_REMAINDER ANSI_FG_RGB(120, 130, 145)

#define COLOR_DIVIDER ANSI_FG_RGB(90, 105, 120)
#endif