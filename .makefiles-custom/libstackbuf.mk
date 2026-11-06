# Static library build. The c skill syntax-checks and runs cppcheck.
# It does not compile or link.

override CFLAGS += -std=c11 -Wall -Wextra -Wpedantic -Werror -Iinclude
override CPPCHECK_FLAGS += -Iinclude --std=c11

STACKBUF_BUILD := build
STACKBUF_LIB := $(STACKBUF_BUILD)/libstackbuf.a
STACKBUF_OBJ := $(STACKBUF_BUILD)/stackbuf.o
STACKBUF_TEST := $(STACKBUF_BUILD)/test_stackbuf

.PHONY: lib test check clean

lib: $(STACKBUF_LIB)

$(STACKBUF_OBJ): src/stackbuf.c include/stackbuf.h
	mkdir -p $(STACKBUF_BUILD)
	$(CC) $(CFLAGS) -c -o $@ src/stackbuf.c

$(STACKBUF_LIB): $(STACKBUF_OBJ)
	$(AR) rcs $@ $(STACKBUF_OBJ)

$(STACKBUF_TEST): tests/test_stackbuf.c include/stackbuf.h $(STACKBUF_LIB)
	$(CC) $(CFLAGS) -o $@ tests/test_stackbuf.c $(STACKBUF_LIB) $(LDFLAGS)

test: $(STACKBUF_TEST)
	./$(STACKBUF_TEST)

check: $(if $(filter c,$(SKILLS)),c-check) test

clean:
	rm -rf $(STACKBUF_BUILD)
