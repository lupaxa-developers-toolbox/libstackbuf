/* Stack buffer.
   The public contract is in include/stackbuf.h. This file holds the buffer
   layout and the function bodies. One buffer is a byte array plus the index
   where the next append writes. That byte is a NUL, so the address is also
   a C string. Append overwrites that NUL and leaves the cursor on the new
   one, which concatenates. Finalise steps past the NUL so the next append
   starts a new string. Mark saves the cursor. Unmark rewinds to it. */

/* Quoted path so a syntax check without -I still finds the public header. */
#include "../include/stackbuf.h"

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* data holds capacity bytes and is always cleared on create and reset.
   cursor is the index of the NUL where the next append writes. cursor may
   equal capacity after finalise fills the last byte. It is never greater. */
struct stackbuf {
    char *data;
    size_t capacity;
    size_t cursor;
};

/* Create a zeroed buffer. A NULL out-pointer or a zero capacity returns
   STACKBUF_INVAL and does not write through buf. Allocation failure sets
   *buf to NULL. */
int stackbuf_create(stackbuf **buf, size_t capacity)
{
    stackbuf *created;
    char *data;

    if (buf == NULL || capacity == 0) {
        return STACKBUF_INVAL;
    }

    created = calloc(1, sizeof(*created));
    data = calloc(capacity, 1);
    if (created == NULL || data == NULL) {
        free(created);
        free(data);
        *buf = NULL;
        return STACKBUF_NOMEM;
    }

    created->data = data;
    created->capacity = capacity;
    created->cursor = 0;
    *buf = created;
    return STACKBUF_OK;
}

/* Free the byte array and the header. *buf is set to NULL. A NULL buffer
   is success. A NULL out-pointer is STACKBUF_INVAL. */
int stackbuf_destroy(stackbuf **buf)
{
    if (buf == NULL) {
        return STACKBUF_INVAL;
    }
    if (*buf == NULL) {
        return STACKBUF_OK;
    }

    free((*buf)->data);
    free(*buf);
    *buf = NULL;
    return STACKBUF_OK;
}

/* Clear every byte and put the cursor back on the first byte. */
int stackbuf_reset(stackbuf *buf)
{
    if (buf == NULL) {
        return STACKBUF_INVAL;
    }

    memset(buf->data, 0, buf->capacity);
    buf->cursor = 0;
    return STACKBUF_OK;
}

/* True when marker addresses a byte of buf at or before the cursor.
   The offset is written to *offset. Pointers into other objects fail. */
static int marker_offset(const stackbuf *buf, const char *marker, size_t *offset)
{
    uintptr_t start;
    uintptr_t pos;
    size_t index;

    if (marker == NULL) {
        return 0;
    }

    start = (uintptr_t)buf->data;
    pos = (uintptr_t)marker;
    if (pos < start) {
        return 0;
    }

    index = (size_t)(pos - start);
    if (index >= buf->capacity || index > buf->cursor) {
        return 0;
    }

    *offset = index;
    return 1;
}

/* Save the cursor. The cursor must still address a byte inside the array,
   which it does not after a finalise that consumed the last byte. */
int stackbuf_mark(stackbuf *buf, char **marker)
{
    if (buf == NULL || marker == NULL) {
        return STACKBUF_INVAL;
    }
    if (buf->cursor >= buf->capacity) {
        return STACKBUF_NOSPACE;
    }

    *marker = buf->data + buf->cursor;
    return STACKBUF_OK;
}

/* Rewind to a marker from this buffer and terminate the string there.
   *marker is cleared so the caller cannot use it as a string afterwards. */
int stackbuf_unmark(stackbuf *buf, char **marker)
{
    size_t offset;

    if (buf == NULL || marker == NULL || *marker == NULL) {
        return STACKBUF_INVAL;
    }
    if (!marker_offset(buf, *marker, &offset)) {
        return STACKBUF_INVAL;
    }

    buf->cursor = offset;
    buf->data[buf->cursor] = '\0';
    *marker = NULL;
    return STACKBUF_OK;
}

/* Format at the cursor. vsnprintf may write a truncated string before we
   know it does not fit, so a failed call puts the NUL back at the cursor.
   Text before the cursor is not modified. */
int stackbuf_append(stackbuf *buf, const char *fmt, ...)
{
    va_list ap;
    char *dest;
    size_t remaining;
    int written;

    if (buf == NULL || fmt == NULL) {
        return STACKBUF_INVAL;
    }
    if (buf->cursor >= buf->capacity) {
        return STACKBUF_NOSPACE;
    }

    dest = buf->data + buf->cursor;
    remaining = buf->capacity - buf->cursor;
    va_start(ap, fmt);
    written = vsnprintf(dest, remaining, fmt, ap);
    va_end(ap);

    if (written < 0) {
        dest[0] = '\0';
        return STACKBUF_INVAL;
    }
    if ((size_t)written >= remaining) {
        dest[0] = '\0';
        return STACKBUF_NOSPACE;
    }

    buf->cursor += (size_t)written;
    return STACKBUF_OK;
}

/* The cursor sits on a NUL. Move it one byte past that NUL and, when a
   byte remains, make that byte a NUL too so the next append has a start. */
int stackbuf_finalise(stackbuf *buf)
{
    if (buf == NULL) {
        return STACKBUF_INVAL;
    }
    if (buf->cursor >= buf->capacity || buf->data[buf->cursor] != '\0') {
        return STACKBUF_NOSPACE;
    }

    buf->cursor++;
    if (buf->cursor < buf->capacity) {
        buf->data[buf->cursor] = '\0';
    }
    return STACKBUF_OK;
}

/* The cursor is at the start only when every mark has been unmarked, or
   the buffer was reset, and nothing has been appended since. */
int stackbuf_verify(const stackbuf *buf)
{
    if (buf == NULL) {
        return STACKBUF_INVAL;
    }
    if (buf->cursor != 0) {
        return STACKBUF_INVAL;
    }
    return STACKBUF_OK;
}

size_t stackbuf_capacity(const stackbuf *buf)
{
    if (buf == NULL) {
        return 0;
    }
    return buf->capacity;
}

size_t stackbuf_used(const stackbuf *buf)
{
    if (buf == NULL) {
        return 0;
    }
    return buf->cursor;
}
