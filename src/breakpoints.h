#pragma once

#ifndef _SPRAY_BREAKPOINTS_H_
#define _SPRAY_BREAKPOINTS_H_

#include <stdlib.h>
#include <stdbool.h>

#include "ptrace.h"

typedef struct Breakpoints Breakpoints;

Breakpoints *init_breakpoints(pid_t pid);

void free_breakpoints(Breakpoints *breakpoints);

/* Enable the given breakpoint by replacing the
   instruction at `addr` with `int 3` (0xcc). This
   will make the child receive a `SIGTRAP` once the
   instruction is reached.
   Creates a new breakpoint at `addr` if it
   didn't exist before. */
void enable_breakpoint(Breakpoints *breakpoints, x86_addr addr);

/* Disable a breakpoint, restoring the original instruction.
   Does nothing if there is no breakpoint at `addr`. */
void disable_breakpoint(Breakpoints *breakpoints, x86_addr addr);

/* Delete the breakpoint at `addr` if there is one. */
void delete_breakpoint(Breakpoints *breakpoints, x86_addr addr);

/* Return `true` if there is a breakpoint at `addr` and
   this breakpoint is enabled. Otherwise, if the breakpoint
   doesn't exist or is disabled, return `false`. */
bool lookup_breakpoint(Breakpoints *breakpoints, x86_addr addr);

#endif // _SPRAY_BREAKPOINTS_H_
