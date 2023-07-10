from subprocess import Popen, PIPE
from typing import Optional
import re
import string
import random

COMMAND = 'build/spray'
SIMPLE = 'tests/assets/linux_x86_bin'
NESTED_FUNCTIONS = 'tests/assets/nested_functions_bin'
MULTI_FILE = 'tests/assets/multi_file_bin'


def random_string() -> str:
    letters = string.ascii_letters
    return ''.join(random.choice(letters) for i in range(8))


def assert_lit(cmd: str, out: str, debugee: Optional[str] = SIMPLE):
    stdout = run_cmd(cmd, debugee)
    assert out in stdout


def assert_ends_with(cmd: str, end: str, debugee: Optional[str] = SIMPLE):
    stdout = run_cmd(cmd, debugee)
    pattern = f'{re.escape(end)}$'
    match = re.search(pattern, stdout, re.MULTILINE)
    if not match:
        print(stdout)
    assert match


def run_cmd(commands: str, debugee: Optional[str] = SIMPLE) -> str:
    p = Popen([COMMAND, debugee], stdout=PIPE, stdin=PIPE, stderr=PIPE)
    output = p.communicate(commands.encode('UTF-8'))
    return output[0].decode('UTF-8')


class TestStepCommands:
    def test_single_step(self):
        assert_lit('b 0x0040116b\nc\ns',
                   '💢 Failed to find another line to step to')

        assert_ends_with('b 0x0040115d\nc\ns\ns\ns\ns\ns\ns', """\
   12 ->   int c = weird_sum(a, b);
   13      return 0;
   14    }
""")
        assert_ends_with('b 0x0040115d\nc\ns\ns', """\
    1    int weird_sum(int a,
    2                  int b) {
    3 ->   int c = a + 1;
    4      int d = b + 2;
    5      int e = c + d;
    6      return e;
""")

    def test_leave(self):
        assert_ends_with('b 0x00401120\nc\nl', """\
   12 ->   int c = weird_sum(a, b);
   13      return 0;
   14    }
""")

    def test_instruction_step(self):
        assert_ends_with('b 0x00401156\nc\ni\ni\ni\ni\ni', """\
    1    int weird_sum(int a,
    2                  int b) {
    3 ->   int c = a + 1;
    4      int d = b + 2;
    5      int e = c + d;
    6      return e;
""")

    def test_next(self):
        assert_ends_with('b 0x0040113d\nc\nn', """\
    2
    3    int add(int a, int b) {
    4      int c = a + b;
    5 ->   return c;
    6    }
    7
    8    int mul(int a, int b) {
""", NESTED_FUNCTIONS)

        assert_lit('n\nn\nn', """\
   17      int sum = add(5, 6);
   18      int product = mul(sum, 3);
   19      printf("Sum: %d; Product: %d\\n", sum, product);
   20 ->   return 0;
   21    }
   22
""", NESTED_FUNCTIONS)


class TestRegisterCommands:
    def test_register_read(self):
        assert_lit('r rip rd', '     rip 0x000000000040114f')
        assert_lit('register rip read', '     rip 0x000000000040114f')

    def test_register_command_errors(self):
        assert_lit('r rax', 'Missing register operation')
        assert_lit('r rax ' + random_string(), 'Invalid register operation')
        assert_lit('r rax rd 0xdeadbeef', 'Trailing characters in command')
        assert_lit('registre rax rd', 'I don\'t know that')

    def test_register_write(self):
        assert_lit('r rax wr 0x123',
                   '     rax 0x0000000000000123 (read after write)')
        assert_lit('register rbx write 0xdeadbeef',
                   '     rbx 0x00000000deadbeef (read after write)')

    def test_register_dump(self):
        dump_end = 'gs 0x00000000000000'
        assert_lit('register dump', dump_end)
        assert_lit('r dump', dump_end)
        assert_lit('r print', dump_end)


class TestMemoryCommands:
    def test_memory_read(self):
        assert_lit('m 0x00007ffff7fe53b0 rd', '         0x00000c98e8e78948')
        assert_lit('memory 0x00007ffff7fe53ba read',
                   '         0xd6894824148b48c4')

    def test_memory_command_errors(self):
        assert_lit('m 0x123', 'Missing memory operation')
        assert_lit('m 0x123 ' + random_string(), 'Invalid memory operation')
        assert_lit('m 0x123 read 0xdeadbeef', 'Trailing characters in command')
        assert_lit('memroy 0x12 wr 0x34', 'I don\'t know that')

    def test_memory_write(self):
        assert_lit('m 0x00007ffff7fe53b0 wr 0xdecafbad',
                   '         0x00000000decafbad (read after write)')
        assert_lit('memory 0x00007ffff7fe53a5 write 0xbadeaffe',
                   '         0x00000000badeaffe (read after write)')


class TestBreakpointCommands:
    def test_breakpoint_set_and_delete(self):
        assert_lit('b 0x00401146\nd 0x00401146\nc', 'Child exited with code 0',
                   NESTED_FUNCTIONS)

    def test_breakpoint_on_function(self):
        assert_ends_with('b weird_sum\nc', """\
Hit breakpoint at address 0x000000000040111a
    1    int weird_sum(int a,
    2                  int b) {
    3 ->   int c = a + 1;
    4      int d = b + 2;
    5      int e = c + d;
    6      return e;
""")
        # This checks that the function `add` has
        # precedence over the address `(0x)add`.
        assert_ends_with('b add\nc', """\
Hit breakpoint at address 0x000000000040113a
    1    #include <stdio.h>
    2
    3    int add(int a, int b) {
    4 ->   int c = a + b;
    5      return c;
    6    }
    7
""", NESTED_FUNCTIONS)

    def test_breakpoint_on_file_line(self):
        assert_ends_with('break tests/assets/file1.c:4\nc', """\
    1    #include "file2.h"
    2
    3    int file1_compute_something(int n) {
    4 ->   int i = 0;
    5      int acc = 0;
    6      while (i < n) {
    7        acc += i * i;
""", MULTI_FILE)

        # Only providing the filename works too,
        # even if the file doesn't exist in the
        # current directory.
        assert_ends_with('break file1.c:4\nc', """\
    1    #include "file2.h"
    2
    3    int file1_compute_something(int n) {
    4 ->   int i = 0;
    5      int acc = 0;
    6      while (i < n) {
    7        acc += i * i;
""", MULTI_FILE)

        # Breakpoints in different files that the
        # entrypoint work, too.
        assert_ends_with('break file2.c:7\nc', """\
Hit breakpoint at address 0x00000000004011b0
    4      if (n < 2) {
    5        return n;
    6      } else {
    7 ->     return   file2_compute_something(n - 1)
    8               + file2_compute_something(n - 2);
    9      }
   10    }
""", MULTI_FILE)

        # Breaking on an empty line falls through
        # to the next line with code on it.
        assert_ends_with('break file2.c:1\nc', """\
Hit breakpoint at address 0x0000000000401190
    1    #include "file2.h"
    2
    3 -> int file2_compute_something(int n) {
    4      if (n < 2) {
    5        return n;
    6      } else {
""", MULTI_FILE)

    def test_breakpoints_delete(self):
        assert_ends_with('break file2.c:4\nc\ndelete file2.c:4\nc', """\
Hit breakpoint at address 0x000000000040119b
    1    #include "file2.h"
    2
    3    int file2_compute_something(int n) {
    4 ->   if (n < 2) {
    5        return n;
    6      } else {
    7        return   file2_compute_something(n - 1)
Child exited with code 0
""", MULTI_FILE)

        assert_ends_with('break add\nc\ndelete add\nc', """\
Hit breakpoint at address 0x000000000040113a
    1    #include <stdio.h>
    2
    3    int add(int a, int b) {
    4 ->   int c = a + b;
    5      return c;
    6    }
    7
Child exited with code 0
""", NESTED_FUNCTIONS)
