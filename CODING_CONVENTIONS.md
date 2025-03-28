# Coding Conventions

## General

* Code shall be [C11](http://www.open-std.org/jtc1/sc22/wg14/www/docs/n1570.pdf)
  compliant, with a list of exceptions detailed below.
* Avoid dynamic memory allocation (malloc/free, new, etc.)! It will break
  real-time guarantees, increase code complexity, and make it more likely to use
  more memory than available.
* Avoid the use of floating point arithmetic. Not every MCU has a FPU and software
  floating point libraries cause unnecessary overhead.
  Instead use fixed-point integers and transform equations so that they stay within
  the range of integer math.
  An easy way to ensure this is by multiplying by a constant factor, ideally a power
  of two - this is a simple shift operation.
  Take care that intermediate values do not exceed the range of the data type you are using.
  When writing drivers, do not convert the measurement data into float, but instead
  choose an appropriate integer format / SI prefix.
* Please obey the Linux coding style as described in
  https://www.kernel.org/doc/Documentation/process/coding-style.rst with the
  following modifications and additions:
    * Line length: aim for no more than 80 characters per line, the absolute
      maximum should be 100 characters per line.
    * All line endings shall be set to LF (`\n`). (How to handle line endings in
      Git: https://help.github.com/articles/dealing-with-line-endings)
    * There must be no trailing whitespace in any line.
      The script `/dist/tools/whitespacecheck/check.sh master || exit` can be
      used to detect whitespaces at the end of line(s) that would lead to
      *Murdock* build error(s).
    * Use empty braces for empty while loops waiting for a hardware register
      instead of semicolon.
      `while (HW_STATUS != STATUS_OK) {}` is correct,
      `while (HW_STATUS != STATUS_OK);` is wrong.
    * Our policy regarding `typedef`s is
      [completely different](https://www.kernel.org/doc/html/latest/process/coding-style.html#typedefs)
      (see below) (BTW: Do we have any reason to do so?)
    * Comments should be C-style comments (see below)
* In order to follow Linux's recommendation on
  [conditional compilation](https://www.kernel.org/doc/html/latest/process/coding-style.html#conditional-compilation)
  make use of `IS_ACTIVE` and `IS_USED` macros from `kernel_defines.h` with C
  conditionals. If a symbol is not going to be defined under a certain
  condition, the usage of preprocessor `#if defined()` is fine.
* You can use [uncrustify](http://uncrustify.sourceforge.net/) with the provided
  option files: https://github.com/RIOT-OS/RIOT/blob/master/uncrustify-riot.cfg

## Standard Compliance

Using extensions to the C standard in general decreases portability and
maintainability: The former because porting RIOT to platforms for which limited
compiler options are available becomes more difficult when compiler-specific
extensions are used. The latter because extensions are often not as clearly
defined as standard C, not as well known within the C development community,
and have fewer resources to look up.

There are a number of cases in which using extensions cannot be avoided, or
would not be maintainable. For these cases, an exception can be made. A list
of recognised exceptions where we can (or even must) rely on extensions include:

- Use of `__attribute__((packed))` is allowed for serialization and
  de-serialization and only there. Ideally, it should not be used in public
  APIs and types.
- Code specific to MCU families may use extensions commonly used in this domain,
  such as inline assembly (e.g. as needed for context swapping), special
  function attributes (e.g. as needed for IRQ vector entries on some MCUs),
  etc. Code should still prefer standard compliance when there is no significant
  downside to it compared to using the extension.
- `#include_next` may be used when system headers need to be extended.
- Function attributes for which a wrapper exists in `compiler_hints.h` may be
  used using that wrapper. These wrappers either unlock additional optimization
  (such as `NORETURN` or `PURE`) or influence warnings (such as `MAYBE_UNUSED`)
  produced by the compiler and can simply be replaced by an empty token for
  compilers that do not support them.
- `__attribute__((used))`, `__attribute__((section("...")))`,
  `__attribute__((weak))`, and `__attribute__((alias("...")))`
  can be used where applicable. Unlike the wrappers in `compiler_hints.h`, we
  actually require toolchain support for them (an empty-token implementation
  will not generate correct binaries).

## Types

* Be careful with platform dependent type sizes like `int` or `long`. Use data
  types that include the bit length in the name like `uint16_t` when you need to
  make sure that a certain variable is exactly of this length.
* The use of typedefs for structs and pointers is allowed.
* Type definitions (using `typedef`) always end on "_t".
* If a typedef is used for a struct, it has to be specified at the struct
  definition (i.e., not as a separate line). E.g.:
```
    typedef struct {
        uint8_t a;
        uint8_t b;
    } foobar_t;
```
* Use of a separate line typedef for structs is allowed for forward
  declarations, e.g.,
```
    typedef struct mystruct mystruct_t;
    [...]
    struct mystruct {
         [...]
    };
```
* Guidelines for pointer types (as long as it is reasonable):
    - use `char *` for strings and only for strings
    - use `uint8_t[]` as type for arbitrary byte buffers, but use `void *` to
      pass them around. `uint8_t[]` because we're dealing with bytes and not
      characters, `void *` to avoid unnecessary casting shall the need arise to
      have their content to have a certain type
    - use `uint8_t *` to pass "typed" byte buffers, e.g., link-layer addresses,
      where it avoids unnecessary temporary variable
    - use `void *` for generic typing

## Variables

* Do NOT use global variables unless it is unavoidable.
* If you declare a variable within a header file, you MUST use the keyword
  `extern`.

## Functions

* Every function needs a prototype in addition to its definition. If a prototype
  is specified within a .c file it has to be declared BEFORE any function
  definitions.
* If the scope of a function is limited to one file, it MUST be declared static.
* Functions without parameters must be specified with `(void)`.
* Keep functions short! As a rule of thumb, the function's body should not
  exceed one screen.
* Do NOT use global macros defining more than one line of code. Use inline
  functions instead.

## Return values

* Every function must return one of the following values or none (void):
    * logical value (zero or not zero)
    * an error code (given as a negative number or zero) or a positive status
      value
    * the count of read or written bytes/values for I/O functions
    * the position or address (for search functions)
    * a pointer
* `NULL` indicates an error case, too.
* Do NOT return structs or other larger types! These would get copied to the
  stack, resulting in expensive operations. Moreover, some compilers have
  trouble with larger return types. Use pointers to structs instead and take
  care of the structs lifetime.
* If possible, prefer signed types over unsigned ones in order to be able to add
  error codes later on.

## Naming

*  Names of all public functions and variables must start with the name of the
   corresponding library, e.g.:
```
    thread_getpid(void);
    hwtimer_init_comp(uint32_t fcpu);
    int transceiver_pid;
```
* Private functions and variables do NOT have this library prefix.
* Do NOT use CamelCase. Function, variable and file names as well as enums,
  structs or typedefs are written in lowercase with underscores.
```
    /* instead of: */
    void CamelCaseNamedFunction(int camelCaseNamedVar);

    /* write: */
    void camel_case_named_function(int camel_case_named_var);
```
* When implementing constants or variables that are defined in third party
  documents such as RFCs, add a prefix to those names based on the RIOT coding
  conventions. If you use a name in the RIOT code that is different from the one
  in the third party document, you must add a reference to the original name of
  the constant or variable in the Doxygen documentation.

## Indentation and braces

* Indentations are four spaces (i.e., NO tab characters).
* As an exception to the Linux coding style, the closing brace is empty on a
  line of its own when followed by an else, too. When followed by a `while` in a
  `do`-statement, it goes into the same line.
* Use curly braces even for one-line blocks. This improves debugging and later
  additions.
```
    /* instead of: */
    if (debug) println("DEBUG");
    else println("DEBUG ELSE");

    /* write: */
    if (debug) {
        println("DEBUG");
    }
    else {
        println("DEBUG ELSE");
    }
```
* Commas are always followed by a space.
* For complex statements it is always good to use more parentheses - or split up
  the statement and simplify it.

## Indentation of Preprocessor Directives

Add two spaces of indent *after* the `#` per level of indent. Increment the
indent when entering conditional compilation using `#if`/`#ifdef`/`#ifndef`
(except for the include guard, which does not add to the indent). Treat indent
for C language statements and C preprocessor directives independently.

```
/* BAD: */
#if XOSC1
#define XOSC XOSC1
#define XOSC_NUM 1
#elif XOSC2
#define XOSC XSOC2
#define XOSC_NUM 2
#endif /* XOSC1/XOSC2 */
```

```
/* GOOD: */
#if XOSC1
#  define XOSC XOSC1
#  define XOSC_NUM 1
#elif XOSC2
#  define XOSC XSOC2
#  define XOSC_NUM 2
#endif
```

```
/* BAD: */
void init_foo(uint32_t param)
{
    (void)param;
    #if HAS_FOO
    switch (param) {
    case CASE1:
        do_foo_init_for_case1;
        break;
    #if HAS_CASE_2
    case CASE2:
        do_foo_init_for_case2;
        break;
        #endif
    #endif
}
```

```
/* GOOD: */
void init_foo(uint32_t param)
{
    (void)param;
#if HAS_FOO
    switch (param) {
    case CASE1:
        do_foo_init_for_case1;
        break;
#  if HAS_CASE_2
    case CASE2:
        do_foo_init_for_case2;
        break;
#  endif
#endif
}
```

### Reasoning

Adding the indent does improve readability a lot, more than adding comments.
Hence, we prefer the indent to allow reviewers to quickly grasp the structure
of the code.

Adding spaces before the `#` is not in compliance with the C standard (even
though in practice compilers will be just fine with whitespace in front), but
adding spaces afterwards is standard compliant. In either case, having the `#`
at the beginning of the line makes it visually stand out from C statements,
which eases reading the code.

Using an indent width of 2 makes preprocessor directives visually more
distinctive from C code, which helps to quickly understand the structure
of code.

## Includes

* The include of system headers (in <>-brackets) always precedes RIOT specific
  includes (in quotes).
* Optional headers must only be included if their corresponding module is
  selected/being build. In other words: always put an `#ifdef MODULE_...`
  statement around includes of optional headers:
```c
#ifdef MODULE_ABC
#  include "abc.h"
#endif
```

### Include What You Use (IWYU)

`#include` directives that are not actually needed should be removed to reduce
clutter and improve compilation speed. Similar: Try to add the corresponding
`#include`s for all the functions, macros, types, etc. used and do not rely on
`bar.h` to implicitly include `foo.h`, unless this is documented behavior.

Tools such as [clang's Include Cleaner][clangd-include-cleaner] can help with
that. These tools may show false positives in cases where headers are *expected*
to be included indirectly: E.g. if `foo.h` is the public header that contains
common helpers and implementations, but a per platform `foo_arch.h` is included
from within `foo.h` for platform specific implementations. If in this scenario
only functions provided by `foo_arch.h` are included, the `#include` of `foo.h`
is considered as unused. To avoid this, one should add
[`/* IWYU pragma: export */`](https://github.com/include-what-you-use/include-what-you-use/blob/master/docs/IWYUPragmas.md) after `#include "foo_arch.h"` in `foo.h`.

[clangd-include-cleaner]: https://clangd.llvm.org/design/include-cleaner

## Header Guards

All files are required to have header guards of the form

```c
#ifndef PATH_TO_FILE_FILENAME_H
#define PATH_TO_FILE_FILENAME_H

...
#endif /* PATH_TO_FILE_FILENAME_H */
```

Rules for generating the guard name:

1. take the file name
2. if there's ```include/``` in the file's pathname, include the path from there
   on.
3. replace "/" and "." with "_"
4. convert to uppercase letters
5. if the produced guard starts with "_", prefix "PRIV"

Examples:

- "core/include/msg.h" -> "MSG_H"
- "sys/include/net/gnrc/pkt.h" -> NET_GNRC_PKT_H
- "drivers/abcd0815/abcd0815_params.h" -> ABCD0815_PARAMS_H
- "sys/module/_internal.h" -> PRIV_INTERNAL_H

Note: these rules will be enforced by the CI.

## C++ compatibility

* C Header files should be always wrapped for C++ compatibility to prevent
  issues with name mangling, i.e. mark all the containing functions and
  definitions as `extern "C"`
``` C
#ifdef __cplusplus
extern "C" {
#endif

... all your function declarations, global variables and defines belong here

#ifdef __cplusplus
}
#endif
```

* use `__restrict` instead of `restrict` in headers (compare
  https://github.com/RIOT-OS/RIOT/pull/2042)

## Absolute values

* Absolute values must be specified as macros or enums, not as literals, i.e.
  instead of
```
int timeout = 7 * 1000000;
```
write
```
int timeout = TIMEOUT_INTERVAL * USEC_PER_SEC;
```
## Comments
* All comments should be written as C-style comments.

E.g:
```
/* This is a C-style comment */
```
Wrong:
```
// C++ comment here
```

## Documentation

* All documentation must be in English.
* All files contain the copyright note and the author.
* Doxygen documentation is mandatory for all header files.
* Every header file and group includes a general description about the provided
  functionality.
* Every symbol must be documented, including function parameters and return values.

Each header and source file must start with a copyright notice.
Note that copyright notices must **not** be Doxygen comments (`/**`).

```c
/*
 * Copyright (C) 2042 Your Name <you@example.org>
 *
 * This file is subject to the terms and conditions of the GNU Lesser General
 * Public License v2.1. See the file LICENSE in the top level directory for more
 * details.
 */
```

### Groups

Documentation in RIOT is organized into Doxygen _groups_. Each group can be part of another group
and can contain other child groups.

To create a **new** group, you use the `@defgroup` command which expects a unique group identifier 
and a human-readable group title. The identifier should be prefixed with the parent group 
identifier followed by an underscore. You should add a new group to an existing parent group with 
the `@ingroup` command. When creating a new group, also give a brief description.

**Example**: You created a module for an Avian Carrier Network Protocol. 
In this case, the Doxygen group for the Avian Carrier Network Protocol would best fit the existing
`sys_net` (System > Networking) group.

```c
/**
 * @defgroup sys_net_avian_carriers Avian Carrier Network Protocol
 * @ingroup  sys_net
 * @brief    Send and receive messages using avian carriers
 * @{
 * 
 * ## Introduction
 * Lorem ipsum.
 * 
 * ## Conclusion
 * Lorem ipsum.
 */
 
/**
 * @}
 */
```

Detailed documentation and symbols (functions, macros, structures, enums, etc.) should be put
inbetween the curly braces following the group definition. You should add a detailed description 
inbetween the curly braces. (If the detailed description grows too large,
[we recommend splitting textual documentation and symbol documentation](#docmd-and-docmd).) 
This also includes files you want to document, like the header itself. 

**Example**: You want to document the current file and a function for sending data.

```c
/**
 * @file
 * @brief  Umbrella header for Avian Carrier Network Protocol
 * @author You <you@example.org>
 */
 
/**
 * @brief Sends data using an avian carrier and blocks until response has been received
 * 
 * @param[in] request Data to send
 * @param[out] response Data received in response
 * @param capacity Capacity of @p response buffer
 * @param[in,out] carrier Avian carrier instance
 *
 * @returns Response size in bytes or negative integer in case of a failure
 */
int avian_carrier_transceive(uint8_t* request, uint8_t* response, size_t capacity, avian_carrier_t* carrier);
```

### Sections

It is strongly recommended to enclose closely related symbols in Doxygen sections. To create
a section, use the `@name` command followed by curly braces. This ensures Doxygen does not create a
long sections of all functions or all macros.

**Example**: Your module exposes multiple functions for initializing an avian carrier and
for sending data.

```c
/**
 * @name Initializing an avian carrier
 * @{
 */
/** ... */
int avian_carrier_init(avian_carrier_t* carrier);

/** ... */
int avian_carrier_init_mode(avian_carrier_t* carrier, avian_navigation_mode_t mode);
/** @} */

/**
 * @name Sending requests using an avian carrier
 * @{
 */
/** ... */
int avian_carrier_send(avian_carrier_t* carrier, 
                       uint8_t* request, size_t size);

/** ... */
int avian_carrier_send_recv(avian_carrier_t* carrier, 
                            uint8_t* request, size_t size, 
                            uint8_t* response, size_t capacity);
/** @} */
``` 

### `@addtogroup` and `@ingroup`

If you want to _add_ documentation to an existing group, use the `@addtogroup` **or**
`@ingroup` command. A common use case for `@addtogroup` is documentation and symbols in other
headers. If there are many symbols to add to the group, you should use `@addtogroup`
followed by curly braces enclosing the symbols and their documentation comments. Provided only very 
few symbols are supposed to be added to the group, you can add `@ingroup` to each individual Doxygen 
comment. With more than one symbol that needs to be added, `@addtogroup` is recommended. 
Remember to create logical sections using `@name` when using `@addtogroup`, too.

**Example**:

```c
/**
 * @addtogroup sys_net_avian_carriers
 * @{
 */ 
/**
 * @name Customizing an avian carrier's average speed
 * @{
 */
/** @brief ... */
int avian_carrier_set_velocity(avian_carrier_t* carrier, int velocity);

/** @brief ... */
int avian_carrier_get_velocity(avian_carrier_t* carrier);
/** @} */

/**
 * @name Modifying an avian carrier's navigation mode
 * @{
 */
/** @brief ... */
int avian_carrier_set_navigation_mode(avian_carrier_t* carrier, avian_navigation_mode_t mode);

/** @brief ... */
avian_navigation_mode_t avian_carrier_get_navigation_mode(avian_carrier_t* carrier);
/** @} */

/** @} */
```

**Example**: Single comment outside group definition, without enclosing `@addtogroup` comment:

```c
/**
 * @file
 * @ingroup sys_net_avian_carriers_multipath
 * @brief   Multipath support for Avian Carrier Network Protocol
 * @author  You <you@example.org>
 */
```

### `doc.md` and `*.doc.md`

If you have a larger detailed description for your group, consider moving the group
definition with its longer description into a Markdown file named `doc.md` or `*.doc.md` close to
the source. The Markdown file can be located anywhere in the RIOT tree, but it is recommeneded not to 
put documentation Markdown files in `include` directories in favor of source directories.
If the directory you would like to put the documentation Markdown file in a folder that hosts other
content not related to your group, you should use a custom filename that ends in `.doc.md`.

**Example**: In a header, you defined a `sys_net_avian_carriers_multipath` group that contains
a large detailed description and symbols. In this case, you would move the `@defgroup`, 
`@ingroup`, `@brief`, and the following detailed description to a separate `multipath.doc.md` file.
The symbols remain in the header inbetween the curly braces, yet with an added `@addtogroup` command. 

```c
/**
 * @defgroup sys_net_avian_carriers_multipath Multipath Support
 * @ingroup  sys_net_avian_carriers
 * @brief    Use multipath routing in the Avian Carrier Network
 * @{
 * 
 * ## Introduction
 * ...
 */
C Symbols, @file, etc goes here...
/** @} */
```

`multipath.doc.md`:
```md
@defgroup sys_net_avian_carriers_multipath Multipath Support
@ingroup  sys_net_avian_carriers
@brief    Use multipath routing in the Avian Carrier Network

## Introduction
...
```

After moving the detailed description into a separate `*.doc.md` file, symbols in the header must 
be added to the group using `@addtogroup`.

#### Articles

Also use this approach for articles, i.e., plain-Markdown content. 

**Example**: You want to add an article entitled _The Avian Carrier Network Explained_ to the
_System > Networking > Avian Carrier Network Protocol_ group. In this case, create a file
named `explanation.doc.md` in the source directory of the avian carrier module that defines
a Doxygen group. The group is like any other group, except it does not contain symbols.

```md
@defgroup sys_net_avian_carriers_explanation The Avian Carrier Network Explained
@ingroup  sys_net_avian_carriers
@brief    Read about the evolution and innerworkings of the Avian Carrier Network

## Introduction
Lorem ipsum.

## ...
```

#### Images

When adding images to `doc.md` or `*.doc.md` files, put the image in a location close to the 
Markdown file. Then reference, the file from the Markdown file. You must add the image path
to `HTML_EXTRA_FILES` in the Doxygen's configuration located at `doc/doxygen/riot.doxyfile`. 

**Example**: Add `![Schematic of a sample feathered carrier](sample-carrier.svg)` to the Markdown
file and include `HTML_EXTRA_FILES += ../../sys/net/network_layer/avian_carrier/sample-carrier.svg`
to `riot.doxyfile`.

This technique allows images to be shown when viewing these Markdown files on GitHub. 
Adding the image path to `HTML_EXTRA_FILES` is necessary to ensure the image is picked up by Doxygen
and copied to the HTML output directory.

### Common directives

Whenever possible, use [`@ref`](https://www.doxygen.nl/manual/commands.html#cmdref) to link to 
other documentation content, including other groups.

**Examples**: `@ref sys_net_avian_carriers_explanation`, `@ref avian_carrier_t.mode`,
`@ref AVIAN_CARRIER_VERSION`.

Use [`@note`](https://www.doxygen.nl/manual/commands.html#cmdnote), 
[`@warning`](https://www.doxygen.nl/manual/commands.html#cmdwarning), 
[`@remark`](https://www.doxygen.nl/manual/commands.html#cmdremark), and
[`@attention`](https://www.doxygen.nl/manual/commands.html#cmdattention) to create a highlighted 
note, warning, or remark. Use [`@see`](https://www.doxygen.nl/manual/commands.html#cmdsee) to 
create a _See Also_ box.

Annotate functions with [`@returns`](https://www.doxygen.nl/manual/commands.html#cmdreturns) where 
applicable.  [`@pre`](https://www.doxygen.nl/manual/commands.html#cmdpre) and 
[`@post`](https://www.doxygen.nl/manual/commands.html#cmdpost) can help document preconditions and 
postconditions. You can also mark symbols as deprecated using 
[`@deprecated`](https://www.doxygen.nl/manual/commands.html#cmddeprecated).

For more information, please consult the [Doxygen documentation](https://www.doxygen.nl/manual/index.html).

## Common compilation warnings

Some solutions to correctly handle compilation warnings.

### -Wformat

Solution for string formatting errors:

* When printing a `size_t`
    * use `PRIuSIZE` from `architecture.h` because `newlib-nano` does not support `%zu`
* When printing an `unsigned char/uint8_t`
    * Use `%u` because `newlib-nano` does not support `%hu/PRIu8`
      [example](https://github.com/RIOT-OS/RIOT/pull/4851)
* When printing an `uint32_t`
    * Use `PRIu32` print format
      [example](https://github.com/RIOT-OS/RIOT/blob/4c74db4e7a6cf1e3be1edb3c10cdba14ba032513/drivers/sx127x/sx127x_getset.c#L126)
* When printing `64bit` variables
    * It is not correctly supported by `newlib-nano` as
      [said here](https://github.com/RIOT-OS/RIOT/issues/1891). It is
      recommended to use `fmt` module for these.
      [Example](https://github.com/RIOT-OS/RIOT/blob/e19f6463c09fc22c76c5b855799054cf27a697f1/tests/posix_semaphore/main.c#L277)

### -Wformat-nonliteral

For a `printf` style function with the following error: `error: format string is
not a string literal`.

* Function using a variable number of arguments:
    * Use `__attribute__((__format__ (__printf__, 3, 4)))`, where here `3` is
      the number of the argument with the format and `4` the format arguments,
      starting from 1. See
      [example](https://github.com/miri64/RIOT/blob/d6cdf4d06f2aeed05dcf86a5437254e2403e147b/pkg/openthread/contrib/platform_logging.c#L31-L32)
* Function using `va_list`:
    * Use `__attribute__((__format__ (__printf__, 1, 0)))`, where here `1` is
      the number of the argument with the format and `0` as there is no variable
      numbers of arguments. See
      [example](https://github.com/miri64/RIOT/blob/ad133da2096c44e001ee65071cb36db60a54e215/cpu/native/syscalls.c#L268-L271)


## Git

* Make one commit per change.
* The first line of the commit message describes the main feature of the commit.

## Continuous Integration
* If the CI tests fail due to errors these errors need to be addressed.
* If the CI tests fail due to warnings/errors emitted by cppcheck you should try
  to fix the error. If the error is definitely a false positive there is the
  possibility to suppress this warning/error. You MUST do so by adding a
  comment, including a rationale why it is a false positive and why the code
  can't be fixed otherwise, in the following format:
```
    /* cppcheck-suppress <category of error/warning>
     * (reason: cppcheck is being really silly. this is certainly not a
     * null-pointer dereference */
```

## Python coding convention

* Code shall be compliant with Python 3.10 at minimum, because this is the
  default Python 3 version in Ubuntu 22.04 (used as the reference system for
  CI).
* Code shall report no error when running the
  [Flake8](http://flake8.pycqa.org/en/latest/) tool, e.g:
    * for style checks described in
      [PEP 8](https://www.python.org/dev/peps/pep-0008/),
    * for lint checks provided by
      [Pyflakes](https://pypi.python.org/pypi/pyflakes),
    * for complexity checks provided by the
      [McCabe project](https://pypi.python.org/pypi/mccabe)
* A line length of maximum of 120 is allowed instead of 79 as per PEP 8. This
  increases tests readability as they can expects long line of output.
* Only runnable scripts shall start with `#!/usr/bin/env python3`
* Runnable scripts shall use the following scheme:

```python
#!/usr/bin/env python3

# Copyright (C) <your copyright>
#
# This file is subject to the terms and conditions of the GNU Lesser
# General Public License v2.1. See the file LICENSE in the top level
# directory for more details.

# put the module imports first
# see https://www.python.org/dev/peps/pep-0008/#imports
# for more details
import module1
import module2

# Optional global variables
GLOBAL_VARIABLE = "I'm global"


# local functions, if required
def local_func():
    # Put your local function code here


# The main function
def main_func():
    # Put your main code here


if __name__ == "__main__":
    # Call the main function from here:
    main_func()
```
