#include "backtrace.h"

#include "magic.h"
#include "ptrace.h"
#include "registers.h"

#include <assert.h>
#include <stdio.h>

typedef struct {
  x86_addr pc;
  x86_addr frame_pointer;
  struct {
    // If `has_lineno` is false, `lineno` is meaningless.
    // Always check `has_lineno` before using `lineno`.
    bool has_lineno;
    uint32_t lineno;
  };
  const char *function;
} CallLocation;

struct CallFrame {
  CallFrame *caller;
  CallLocation location;
};

CallFrame *init_call_frame(CallFrame *caller, x86_addr pc,
                           x86_addr frame_pointer, DebugInfo *info) {
  const DebugSymbol *func_sym = sym_by_addr(pc, info);

  const char *func_name = NULL;
  const Position *pos = NULL;

  if (func_sym != NULL) {
    func_name = sym_name(func_sym, info);
    pos = sym_position(func_sym, info);
  }

  CallFrame *frame = malloc(sizeof(*frame));
  assert(frame != NULL);

  if (pos != NULL) {
    frame->location.has_lineno = true;
    frame->location.lineno = pos->line;
  } else {
    frame->location.has_lineno = false;
  }

  frame->caller = caller;
  frame->location.pc = pc;
  frame->location.frame_pointer = frame_pointer;
  frame->location.function = func_name;

  return frame;
}

/* Check if the first to instructions of the function belonging
   to the given PC look like this:
    55       push   %rbp
    48 89 e5 mov    %rsp,%rbp
   This is the standard procedure to store the previous functions's
   frame pointer and then set the current function's frame pointer
   to the start of the frame (i.e. the stack pointer right at the
   start of the function). If this isn't found, it's likely that
   the compiler omitted the frame pointer so we should emit a warning. */
// bool stores_frame_pointer(const ElfFile *elf, pid_t pid, x86_addr pc) {
bool stores_frame_pointer(x86_addr pc, pid_t pid, DebugInfo *info) {
  const DebugSymbol *func = sym_by_addr(pc, info);
  if (func == NULL) {
    return false;
  }

  x86_word insts = {0};
  SprayResult mem_res = pt_read_memory(pid, sym_start_addr(func), &insts);
  if (mem_res == SP_ERR) {
    return false;
  }

  // Get the first four bytes only.
  insts.value &= 0xffffffff;
  return insts.value == 0xe5894855;
}

/* NOTE: This is a naive approach to getting a backtrace
   which relies on the compiler emitting a frame pointer.
   Try compiling again with `-fno-omit-frame-pointer` if
   this doesn't work. */

CallFrame *init_backtrace(x86_addr pc, pid_t pid, DebugInfo *info) {
  assert(info != NULL);

  // Get the saved base pointer of the caller.
  x86_addr frame_pointer = {0};
  SprayResult reg_res =
      get_register_value(pid, rbp, (x86_word *)&frame_pointer);
  if (reg_res == SP_ERR) {
    return NULL;
  }

  if (!stores_frame_pointer(pc, pid, info)) {
    printf("WARN: it seems like this executable doesn't maintain a frame "
           "pointer.\n"
           "      This results in incorrect or incomplete backtraces.\n"
           "HINT: Try to compile again with `-fno-omit-frame-pointer`.\n\n");
  }

  CallFrame *call_frame = init_call_frame(NULL, pc, frame_pointer, info);

  while (frame_pointer.value != 0) {
    // Read the return address of the current function
    // and use it as the PC of the next function.
    // NOTE: This operation must be performed *before* the
    // frame pointer is updated.
    SprayResult ret_res = pt_read_memory(
        pid, (x86_addr){frame_pointer.value + 8}, (x86_word *)&pc);
    if (ret_res == SP_ERR) {
      free_backtrace(call_frame);
      return NULL;
    }

    // Read the frame pointer of the next function.
    SprayResult fp_res =
        pt_read_memory(pid, frame_pointer, (x86_word *)&frame_pointer);
    if (fp_res == SP_ERR) {
      free_backtrace(call_frame);
      return NULL;
    }

    call_frame = init_call_frame(call_frame, pc, frame_pointer, info);
  }

  return call_frame;
}

void free_backtrace(CallFrame *call_frame) {
  // Recursively free all call frames.
  while (call_frame != NULL) {
    CallFrame *caller = call_frame->caller;
    free(call_frame);
    call_frame = caller;
  }
}

void print_backtrace(CallFrame *call_frame) {
  printf("How did we even get here? (backtrace)\n");
  if (call_frame == NULL) {
    printf("<empty backtrace>\n");
  } else {
    CallLocation *location;
    while (call_frame != NULL) {
      location = &call_frame->location;

      printf("  ");
      print_addr(location->pc);
      printf(" ");

      if (location->function) {
        printf("%s", location->function);
      } else {
        printf("<?>");
      }

      if (location->has_lineno) {
        printf(":%u\n", location->lineno);
      } else {
        printf("\n");
      }

      call_frame = call_frame->caller;
    }
  }
}
