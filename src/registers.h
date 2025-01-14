#pragma once

#ifndef _SPARY_REGISTERS_H_
#define _SPARY_REGISTERS_H_

#include "magic.h"
#include "ptrace.h"

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

typedef enum {
  r15=0, r14, r13, r12,
  rbp, rbx, r11, r10, r9, r8,
  rax, rcx, rdx, rsi, rdi,
  orig_rax, rip, cs, eflags,
  rsp, ss, fs_base, gs_base,
  ds, es, fs, gs,
} x86_reg;

typedef struct {
  x86_reg r;
  int dwarf_r;  // DWARF register number.
  const char *name;
} reg_descriptor;

static const reg_descriptor reg_descriptors[N_REGISTERS] = {
  { r15, 15, "r15" },
  { r14, 14, "r14" },
  { r13, 13, "r13" },
  { r12, 12, "r12" },
  { rbp, 6, "rbp" },
  { rbx, 3, "rbx" },
  { r11, 11, "r11" },
  { r10, 10, "r10" },
  { r9, 9, "r9" },
  { r8, 8, "r8" },
  { rax, 0, "rax" },
  { rcx, 2, "rcx" },
  { rdx, 1, "rdx" },
  { rsi, 4, "rsi" },
  { rdi, 5, "rdi" },
  { orig_rax, -1, "orig_rax" },
  { rip, -1, "rip" },
  { cs, 51, "cs" },
  { eflags, 49, "eflags" },
  { rsp, 7, "rsp" },
  { ss, 52, "ss" },
  { fs_base, 58, "fs_base" },
  { gs_base, 59, "gs_base" },
  { ds, 53, "ds" },
  { es, 50, "es" },
  { fs, 54, "fs" },
  { gs, 55, "gs" },
};

SprayResult get_register_value(pid_t pid, x86_reg reg, x86_word *store);
SprayResult set_register_value(pid_t pid, x86_reg reg, x86_word word);

/* If `dwarf_regnum` refers to a known register, then
   `true` is returned and `store` is set to the  content
   of that register. Otherwise this function returns
   `false` and `store` stays untouched. */
bool get_dwarf_register_value(pid_t pid, int8_t dwarf_regnum, x86_word *store);

const char *get_name_from_register(x86_reg reg);
/* If `name` refers to a known register, then
   `true` is returned and `store` is set to that register.
   Otherwise this function returns `false` and `store` stays untouched. */
bool get_register_from_name(const char *name, x86_reg *store);

#endif  // _SPARY_REGISTERS_H_
