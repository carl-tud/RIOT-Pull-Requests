/*
 * SPDX-FileCopyrightText: 2026 TU Dresden
 * SPDX-License-Identifier: LGPL-2.1-only
 */

#pragma once

/**
 * @defgroup core_macros_ansi_sgr ANSI Select Graphic Rendition (SGR) for styled terminal text
 * @ingroup core_macros
 * @brief Macros to apply color and style to text in supporting terminal emulators
 * @{
 * Use @ref ANSI_DISPLAY to customize text and background color, color intensities, and
 * text styles, such as bold or underlined text.
 * 
 * ```c
 * const char* s = ANSI_DISPLAY(RED, FOREGROUND, BOLD) "Error!" ANSI_DISPLAY_RESET;
 * ```
 */

/**
 * @file
 * @brief ANSI SGR definitions
 * @author Carl Seifert <carl.seifert@tu-dresden.de>
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name SGR escape sequence
 * @{
 */

/** @brief ANSI escape prefix */
#define _ANSI_SGR_PREFIX         "\x1b["

/** @brief ANSI color/style suffix */
#define _ANSI_SGR_SUFFIX         "m"

/** @} */

/**
 * @name Color codes
 * @{
 */

/** @brief ANSI color code for primary color (black in light appearance, white in dark) */
#define _ANSI_COLOR_CODE_PRIMARY "0"

/** @brief Alias for @ref _ANSI_COLOR_CODE_PRIMARY */
#define _ANSI_COLOR_CODE_BLACK   _ANSI_COLOR_CODE_PRIMARY

/** @brief ANSI color code for red */
#define _ANSI_COLOR_CODE_RED     "1"

/** @brief ANSI color code for green */
#define _ANSI_COLOR_CODE_GREEN   "2"

/** @brief ANSI color code for yellow */
#define _ANSI_COLOR_CODE_YELLOW  "3"

/** @brief ANSI color code for blue */
#define _ANSI_COLOR_CODE_BLUE    "4"

/** @brief ANSI color code for purple (pink, magenta) */
#define _ANSI_COLOR_CODE_PURPLE  "5"

/** @brief ANSI color code for cyan (light blue) */
#define _ANSI_COLOR_CODE_CYAN    "6"

/** @brief ANSI color code for wite (gray) */
#define _ANSI_COLOR_CODE_WHITE   "7"

/** @} */

/**
 * @name Modifiers
 * @{
 */

/** @brief ANSI modifier to make color apply to foreground text */
#define _ANSI_COLOR_MODIFIER_FOREGROUND        "3"

/** @brief ANSI modifier to make color apply to foreground text in high intensity */
#define _ANSI_COLOR_MODIFIER_BRIGHT_FOREGROUND "9"

/** @brief ANSI modifier to make color apply to background */
#define _ANSI_COLOR_MODIFIER_BACKGROUND        "4"

/** @brief ANSI modifier to make color apply to background in high intensity */
#define _ANSI_COLOR_MODIFIER_BRIGHT_BACKGROUND "10"

/** @} */

/**
 * @name Text styles
 * @{
 */

#ifndef DOXYGEN
#  define ANSI_STYLE_NONE                       "0"
#endif

/** 
 * @brief ANSI style for bold text 
 *
 * Must not be combined with @ref ANSI_STYLE_DIM
 */
#define ANSI_STYLE_BOLD                       "1"

/** 
 * @brief ANSI style dimmer, thinner, less prominent text 
 *
 * Must not be combined with @ref ANSI_STYLE_BOLD
 */
#define ANSI_STYLE_DIM                        "2"

/** @brief ANSI style for italic text */
#define ANSI_STYLE_ITALIC                     "2"
/** @brief ANSI style for underlined text */
#define ANSI_STYLE_UNDERLINED                 "4"

/** @brief ANSI style for slowly blinking text */
#define ANSI_STYLE_BLINK_SLOW                 "5"
/** @brief ANSI style for fast-blinking text */
#define ANSI_STYLE_BLINK_FAST                 "6"

/** @brief ANSI style swapping foreground and background color settings */
#define ANSI_STYLE_SWAP_FOREGROUND_BACKGROUND "7"

/** @brief ANSI style for concealed (redacted) text */
#define ANSI_STYLE_CONCEALED                  "8"

/** @brief ANSI style for crossed out text (strikethrough) */
#define ANSI_STYLE_STRIKETHROUGH              "9"

/** @} */

/**
 * @name Formatting macros
 * @{
 */

#ifndef DOXYGEN
/* These macros apply multiple SGR text styles given as suffixes of their respective 
 * ANSI_STYLE_... macro, so styling macros at the expansion site are kept short.
 * Example: __ANSI_STYLE_2(BOLD, UNDERLINED) => ";"  ANSI_STYLE_BOLD ";" ANSI_STYLE_UNDERLINED 
 * __ANSI_GET_STYLE implements variadic style parameters, so ANSI_DISPLAY can be given
 * anywhere between zero and 5 style macros. This limit may be increased in the future. */

/* __ANSI_GET_STYLE returns the correct concatenation macro (__ANSI_STYLE_N) based on the 
 * number of arguments. */
#  define __ANSI_GET_STYLE(_1, _2, _3, _4, _5, name, ...) name
/* __ANSI_STYLED builds the longer style macro from just the suffix token. */
#  define __ANSI_STYLED(keyword)        ANSI_STYLE_ ## keyword
/* These macros recursively expand 1 to 5 style arguments. */
#  define __ANSI_STYLE_1(a)             ";" __ANSI_STYLED(a)
#  define __ANSI_STYLE_2(a, b)          __ANSI_STYLE_1(a)          ";" __ANSI_STYLED(b)
#  define __ANSI_STYLE_3(a, b, c)       __ANSI_STYLE_2(a, b)       ";" __ANSI_STYLED(c)
#  define __ANSI_STYLE_4(a, b, c, d)    __ANSI_STYLE_3(a, b, c)    ";" __ANSI_STYLED(d)
#  define __ANSI_STYLE_5(a, b, c, d, e) __ANSI_STYLE_4(a, b, c, d) ";" __ANSI_STYLED(e)

/* This macro calls __ANSI_GET_STYLE and expands to the concatenated SGR
 * style string (";" .. ";" ...) */
#  define __ANSI_APPLY_STYLES(...) \
    __ANSI_GET_STYLE(__VA_ARGS__, \
        __ANSI_STYLE_5, __ANSI_STYLE_4, __ANSI_STYLE_3, __ANSI_STYLE_2, __ANSI_STYLE_1 \
    )(__VA_ARGS__)

#endif

/**
 * @brief Builds string literal a terminal emulator to apply color and style to following text
 *
 * This macro uses ANSI Select Graphic Rendition (SGR) codes to instruct terminal emulators
 * that support SGR to apply custom styling to subsequent text. 
 * You output this macro before any text, e.g., by calling `puts` or `printf` separately,
 * or by using static string literal concatenation in C. For example, this is how you would
 * format "Hello, World!" such that it appears purple, bold, and underlined. You must append
 * @ref ANSI_DISPLAY_RESET to return to the default terminal text format.
 *
 * ```c
 * const char* message = 
 *   ANSI_DISPLAY(PURPLE, FOREGROUND, BOLD, UNDERLINED) "Hello, World!" ANSI_DISPLAY_RESET;
 *
 * puts(message); // or
 * printf(message "\n");
 * ```
 *
 * You can also override/chain the current format in the string using static string concatenation.
 *
 * ```c
 * const char* message = 
 *   ANSI_DISPLAY(PURPLE, FOREGROUND, BOLD) "styled" ANSI_DISPLAY(YELLOW, BACKGROUND, ITALIC) \
 *   "styled" ANSI_DISPLAY_RESET;
 * ```
 * 
 * @param color The color to apply to following text, e.g., `PURPLE`
 * @param modifier The modifier to select how to apply color: `FOREGROUND` or `BACKGROUND`
 * @param ... Variadic text style arguments, e.g., `BOLD`, `UNDERLINED`
 *
 * ## Color codes
 * - `PRIMARY` is black on white/light terminal backgrounds/themes and white in black/dark themes
 * - `RED`
 * - `GREEN`
 * - `YELLOW`
 * - `BLUE`
 * - `CYAN`
 * - `WHITE` may appear gray
 *
 * ## Color modifiers
 * - `FOREGROUND` makes @p color apply to the foreground, i.e., the text itself
 * - `BRIGHT_FOREGROUND` makes @p color appear brighter and is applied to the foreground
 * - `BACKGROUND` makes @p color apply to the background
 * - `BRIGHT_BACKGROUND` makes @p color appear brighter and is applied to the background
 *
 * ## Text styles
 * - `BOLD` makes text appear thicker
 * - `DIM` makes text appear lighter or less intense, i.e., darker
 *
 * The previous two styles are mutually exclusive (according to ANSI SGR). The following
 * may not be supported by every terminal emulator.
 *
 * - `ITALIC`
 * - `UNDERLINED`
 * - `STRIKETHROUGH` makes text appear crossed out
 * - `CONCEALED` hides the following text, but is still selectable
 * - `BLINK_FAST` makes text blink fast
 * - `BLINK_SLOW` makes text blink slowly
 * - `SWAP_FOREGROUND_BACKGROUND` swaps foreground and background colors
 *
 * You may apply multiple styles, currently up to 5.
 *
 * ```c
 * ANSI_DISPLAY(CYAN, BRIGHT_FOREGROUND, BOLD, UNDERLINED)
 * ```
 *
 * @returns Format string literal
 */
#define ANSI_DISPLAY(color, modifier, ...) \
    _ANSI_SGR_PREFIX \
    _ANSI_COLOR_MODIFIER_ ## modifier \
    _ANSI_COLOR_CODE_ ## color \
    __ANSI_APPLY_STYLES(__VA_ARGS__) \
    _ANSI_SGR_SUFFIX

/**
 * @brief Resets text color and style applied previously back to defaults
 *
 * Append this escape sequence after an styles applied using @ref ANSI_DISPLAY.
 */
#define ANSI_DISPLAY_RESET        _ANSI_SGR_PREFIX ANSI_STYLE_NONE _ANSI_SGR_SUFFIX

/**
 * @brief   ANSI color escape code for black
 */
#define ANSI_COLOR_BLACK        ANSI_DISPLAY(PRIMARY, FOREGROUND)

/**
 * @brief   ANSI color escape code for bold black
 */
#define ANSI_COLOR_BLACK_BOLD   ANSI_DISPLAY(PRIMARY, FOREGROUND, BOLD)

/**
 * @brief   ANSI color escape code for red
 */
#define ANSI_COLOR_RED          ANSI_DISPLAY(RED, FOREGROUND)

/**
 * @brief   ANSI color escape code for bold red
 */
#define ANSI_COLOR_RED_BOLD     ANSI_DISPLAY(RED, FOREGROUND, BOLD)

/**
 * @brief   ANSI color escape code for green
 */
#define ANSI_COLOR_GREEN        ANSI_DISPLAY(GREEN, FOREGROUND)

/**
 * @brief   ANSI color escape code for bold green
 */
#define ANSI_COLOR_GREEN_BOLD   ANSI_DISPLAY(GREEN, FOREGROUND, BOLD)

/**
 * @brief   ANSI color escape code for yellow
 */
#define ANSI_COLOR_YELLOW       ANSI_DISPLAY(YELLOW, FOREGROUND)

/**
 * @brief   ANSI color escape code for bold yellow
 */
#define ANSI_COLOR_YELLOW_BOLD  ANSI_DISPLAY(YELLOW, FOREGROUND, BOLD)

/**
 * @brief   ANSI color escape code for blue
 */
#define ANSI_COLOR_BLUE         ANSI_DISPLAY(BLUE, FOREGROUND)

/**
 * @brief   ANSI color escape code for bold blue
 */
#define ANSI_COLOR_BLUE_BOLD    ANSI_DISPLAY(BLUE, FOREGROUND, BOLD)

/**
 * @brief   ANSI color escape code for magenta
 */
#define ANSI_COLOR_MAGENTA       ANSI_DISPLAY(PURPLE, FOREGROUND)

/**
 * @brief   ANSI color escape code for bold magenta
 */
#define ANSI_COLOR_MAGENTA_BOLD ANSI_DISPLAY(PURPLE, FOREGROUND, BOLD)

/**
 * @brief   ANSI color escape code for cyan
 */
#define ANSI_COLOR_CYAN         ANSI_DISPLAY(CYAN, FOREGROUND)

/**
 * @brief   ANSI color escape code for bold cyan
 */
#define ANSI_COLOR_CYAN_BOLD    ANSI_DISPLAY(CYAN, FOREGROUND, BOLD)

/**
 * @brief   ANSI color escape code for white
 */
#define ANSI_COLOR_WHITE        ANSI_DISPLAY(WHITE, FOREGROUND)

/**
 * @brief   ANSI color escape code for bold white
 */
#define ANSI_COLOR_WHITE_BOLD   ANSI_DISPLAY(WHITE, FOREGROUND, BOLD)

/**
 * @brief   ANSI color escape code for resetting
 */
#define ANSI_COLOR_RESET        ANSI_DISPLAY_RESET

/** @} */ /* end of section */

#ifdef __cplusplus
}
#endif

/** @} */ /* end of group */
