#ifndef STACKBUF_H
#define STACKBUF_H

#include <stddef.h>

#define STACKBUF_VERSION_STRING "0.1.0"

/* STACKBUF_OK is success. STACKBUF_NOMEM means allocation failed and
   stackbuf_create sets *buf to NULL. STACKBUF_INVAL means a missing buffer,
   a missing marker, a zero capacity, or a marker that is not in that buffer.
   STACKBUF_NOSPACE means the formatted text would not fit. The buffer is
   left unchanged. */
#define STACKBUF_OK 0
#define STACKBUF_NOMEM (-1)
#define STACKBUF_INVAL (-2)
#define STACKBUF_NOSPACE (-3)

#ifdef __cplusplus
extern "C" {
#endif

typedef struct stackbuf stackbuf;

/* Allocate a zeroed buffer of capacity bytes. capacity must be at least 1.
   On success *buf receives the buffer and the cursor is at the first byte.
   On allocation failure *buf is NULL. A zero capacity leaves *buf unchanged. */
int stackbuf_create(stackbuf **buf, size_t capacity);

/* Free the buffer and set *buf to NULL. Destroying an already NULL buffer
   is success. Markers into this buffer are invalid after this returns. */
int stackbuf_destroy(stackbuf **buf);

/* Rewind the cursor to the start and clear every byte. Markers into this
   buffer are invalid after this returns. */
int stackbuf_reset(stackbuf *buf);

/* Store the cursor address in *marker. The byte there is a NUL, and it is
   the start of the text the next append writes. */
int stackbuf_mark(stackbuf *buf, char **marker);

/* Rewind the cursor to *marker, write a NUL there, and set *marker to NULL.
   The pointer must address a byte in this buffer at or before the cursor.
   An empty mark, whose byte is already NUL, is valid. */
int stackbuf_unmark(stackbuf *buf, char **marker);

/* Format text at the cursor. The write starts on the NUL that already sits
   there, so a later append concatenates. The cursor is left on the new NUL.
   On failure the bytes and the cursor are unchanged. */
int stackbuf_append(stackbuf *buf, const char *fmt, ...);

/* Step one byte past the NUL at the cursor. The finished string stays
   readable at its start address. The next append starts a new string. */
int stackbuf_finalise(stackbuf *buf);

/* Return STACKBUF_OK when the cursor is at the first byte. */
int stackbuf_verify(const stackbuf *buf);

size_t stackbuf_capacity(const stackbuf *buf);
size_t stackbuf_used(const stackbuf *buf);

#ifdef __cplusplus
}
#endif

#endif /* STACKBUF_H */
