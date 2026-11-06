<p align="center">
  <a href="https://github.com/lupaxa-developers-toolbox">
    <img src="https://raw.githubusercontent.com/the-lupaxa-project/brand-assets/master/logos/organisations/developers-toolbox/readme-logo.png" alt="Developers Toolbox" />
  </a>
</p>

<h1 align="center">Libstackbuf</h1>

C library for a stack buffer used as temporary string storage.

Each buffer is a byte array and a cursor. Append formats text at the cursor
and concatenates. Finalise steps past the terminating NUL so the next append
starts a new string. Mark saves the cursor, and unmark rewinds to it. The
buffer type is opaque. The public header is `include/stackbuf.h`. It uses C
linkage, so a C++ translation unit can include it.

## Requirements

- GNU make.
- gcc, unless `CC` is set. The C skill uses gcc for syntax checks.
- cppcheck, for `make c-check`.

## Build

`make lib` writes the static archive `build/libstackbuf.a`. The archive is
built as C11 with `-Wall -Wextra -Wpedantic -Werror`. The compiler is gcc
unless `CC` is set. `make clean` removes `build/`.

```bash
make lib
cc -std=c11 -Iinclude -o demo demo.c build/libstackbuf.a
```

`make lib CC=clang` selects the compiler. `make lib CFLAGS=-g` adds flags to
that C11 build. `make` with no target prints the makefile help.

`make update` refreshes the makefile skills. `make lib` and `make test` do
not need that checkout. `make c-check` and `make version` do.

## Quick Start

Create a 64-byte buffer, append two pieces of text, and read them back from
the mark. `stackbuf_destroy` frees the buffer.

```c
#include "stackbuf.h"

#include <stdio.h>

int main(void)
{
    stackbuf *buf = NULL;
    char *text = NULL;

    if (stackbuf_create(&buf, 64) != STACKBUF_OK) {
        return 1;
    }

    if (stackbuf_mark(buf, &text) != STACKBUF_OK) {
        stackbuf_destroy(&buf);
        return 1;
    }

    stackbuf_append(buf, "This");
    stackbuf_append(buf, " is a test");
    printf("%s\n", text);

    stackbuf_unmark(buf, &text);
    stackbuf_destroy(&buf);
    return 0;
}
```

## How the Cursor Moves

The cursor is the index of a NUL. An append writes over that NUL, so the next
append extends the same string, and the cursor stays on the new NUL.

`stackbuf_finalise` moves the cursor one byte past that NUL. The finished
string stays readable. The next append starts a different string after it.

`stackbuf_mark` stores the cursor address. `stackbuf_unmark` puts the cursor
back there, writes a NUL, and sets the caller's pointer to NULL. Text written
after the mark is then open to being overwritten. The bytes stay until a later
append or a reset replaces them.

A mark taken while the cursor is on a NUL is an empty string. Unmark of that
mark succeeds.

`stackbuf_reset` clears every byte and puts the cursor on the first byte.
Marks from before the reset are invalid.

> **Note:** Callers serialise use of a given buffer. The mark is an address
> inside that buffer.

## Return Codes

Every function that can fail returns an `int`.

| Code               | Value | Meaning                                                                            |
| ------------------ | ----- | ---------------------------------------------------------------------------------- |
| `STACKBUF_OK`      | `0`   | The call succeeded.                                                                |
| `STACKBUF_NOMEM`   | `-1`  | Allocation failed. `stackbuf_create` sets `*buf` to NULL.                          |
| `STACKBUF_INVAL`   | `-2`  | The buffer or marker is missing, the capacity is 0, or the marker is out of range. |
| `STACKBUF_NOSPACE` | `-3`  | The text would not fit, or the cursor is at the end. The buffer is unchanged.      |

`stackbuf_destroy(NULL)` returns `STACKBUF_INVAL`. `stackbuf_destroy` on a
pointer that is already NULL returns `STACKBUF_OK` and leaves it NULL.

`stackbuf_create` with a zero capacity leaves `*buf` unchanged. On allocation
failure it sets `*buf` to NULL.

A failed append leaves the existing string and the cursor unchanged. A later
shorter append can still succeed.

## Functions

```c
typedef struct stackbuf stackbuf;

int stackbuf_create(stackbuf **buf, size_t capacity);
int stackbuf_destroy(stackbuf **buf);
int stackbuf_reset(stackbuf *buf);
int stackbuf_mark(stackbuf *buf, char **marker);
int stackbuf_unmark(stackbuf *buf, char **marker);
int stackbuf_append(stackbuf *buf, const char *fmt, ...);
int stackbuf_finalise(stackbuf *buf);
int stackbuf_verify(const stackbuf *buf);
size_t stackbuf_capacity(const stackbuf *buf);
size_t stackbuf_used(const stackbuf *buf);
```

| Name                 | Purpose                                                              |
| -------------------- | -------------------------------------------------------------------- |
| `stackbuf_create`    | Allocate a zeroed buffer and store it in `*buf`.                     |
| `stackbuf_destroy`   | Free the buffer and set `*buf` to NULL.                              |
| `stackbuf_reset`     | Clear the bytes and rewind the cursor to the start.                  |
| `stackbuf_mark`      | Store the cursor address in `*marker`.                               |
| `stackbuf_unmark`    | Rewind to `*marker`, write a NUL, and set `*marker` to NULL.         |
| `stackbuf_append`    | Format text at the cursor. A later append concatenates.              |
| `stackbuf_finalise`  | Step past the current NUL so the next append starts a new string.    |
| `stackbuf_verify`    | Succeed when the cursor is at the first byte.                        |
| `stackbuf_capacity`  | Return the byte capacity. A NULL buffer returns 0.                   |
| `stackbuf_used`      | Return the cursor index. A NULL buffer returns 0.                    |

`stackbuf_append` uses the same format string as `printf`. One byte of the
capacity is the terminating NUL of the string being written. A request that
does not leave room for that NUL returns `STACKBUF_NOSPACE`.

`stackbuf_unmark` accepts a pointer from `stackbuf_mark` on this buffer. A
pointer from another buffer, a NULL pointer, or an address past the cursor
returns `STACKBUF_INVAL` and changes nothing.

## Marks

Build an inner string from the current cursor, read it, then rewind so the
outer string can continue.

```c
char *outer = NULL;
char *inner = NULL;

stackbuf_mark(buf, &outer);
stackbuf_append(buf, "This");
stackbuf_append(buf, " is a");

stackbuf_mark(buf, &inner);
stackbuf_append(buf, "This is a test");
stackbuf_finalise(buf);
stackbuf_unmark(buf, &inner);

stackbuf_append(buf, " test also");
stackbuf_finalise(buf);
```

After the inner unmark, `outer` reads `This is a test also`.

`stackbuf_finalise` before the unmark leaves the inner text terminated so it
can be read. The unmark then puts the cursor back on the start of that text.
The following outer append overwrites it.

## Full Buffer

`stackbuf_finalise` may leave the cursor just past the last byte. Further
appends and marks return `STACKBUF_NOSPACE`. `stackbuf_reset` makes the whole
capacity usable again.

A buffer of one byte holds an empty string and nothing more.

## Empty Buffer

A new buffer has a cursor of 0, and `stackbuf_verify` succeeds. Destroying it
returns `STACKBUF_OK` and sets the caller's pointer to NULL. Destroying that
NULL pointer again returns `STACKBUF_OK`.

`stackbuf_capacity(NULL)` and `stackbuf_used(NULL)` return 0.

## Version

The version text is the macro `STACKBUF_VERSION_STRING` in
`include/stackbuf.h`. It is a dotted semantic version. `.bumpversion.toml`
is the version source, and a bump updates that macro.

After `make init`, `make version` prints the version from
`.bumpversion.toml`.

## Development

```bash
make update
make lib
make check
make clean
```

`make c-check` syntax-checks every `.c` and `.h` file, then runs cppcheck.
`make check` runs `c-check` and then the behaviour tests. `make test` runs
only the behaviour tests.

<a href="https://github.com/the-lupaxa-project">
    <img src="https://raw.githubusercontent.com/the-lupaxa-project/brand-assets/master/logos/components/footer-for-child-orgs.svg" alt="The Lupaxa Project Footer" width="100%" />
</a>
