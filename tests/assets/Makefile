CC = clang
CFLAGS = -g

.PHONY = all clean

SIMPLE = simple.c
NESTED = nested_functions.c
MULTI_FILE = multi-file/file1.c multi-file/file2.c
PRINT_ARGS = print_args.c
COMMENTED = commented.c
TARGETS = 64bit-linux-simple.bin 32bit-linux-simple.bin nested-functions.bin multi-file.bin print-args.bin frame-pointer-nested-functions.bin no-frame-pointer-nested-functions.bin commented.bin

all: $(TARGETS)

64bit-linux-simple.bin: $(SIMPLE)
	$(CC) $(CFLAGS) $< -o $@
32bit-linux-simple.bin: $(SIMPLE)
	$(CC) $(CFLAGS) -m32 $< -o $@
nested-functions.bin: $(NESTED)
	$(CC) $(CFLAGS) $< -o $@
frame-pointer-nested-functions.bin: CFLAGS += -fno-omit-frame-pointer
frame-pointer-nested-functions.bin: $(NESTED)
	$(CC) $(CFLAGS) $< -o $@
no-frame-pointer-nested-functions.bin: CFLAGS += -fomit-frame-pointer
no-frame-pointer-nested-functions.bin: $(NESTED)
	$(CC) $(CFLAGS) $< -o $@
multi-file.bin: $(MULTI_FILE)
	$(CC) $(CFLAGS) $(MULTI_FILE) -o $@
print-args.bin: $(PRINT_ARGS)
	$(CC) $(CFLAGS) $< -o $@
commented.bin: $(COMMENTED)
	$(CC) $(CFLAGS) $< -o $@

clean:
	$(RM) $(TARGETS)

