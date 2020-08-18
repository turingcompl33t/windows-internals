## Windows Shellcode Utilities

Commandline utilities for writing, loading, and executing Windows 64-bit shellcode.

### `ShellcodeWriter`

Accepts x86-64 (64-bit) assembly files (.asm) as input, assembles with MASM, and dumps raw shellcode to specified output file.

**Dependencies**

`ShellcodeWriter` utilizes the `PeLib` Portable Executable parsing library to handle the parsing of the temporary executable file produced by MASM.

### `ShellcodeRunner`

Accepts input file containing raw shellcode (e.g. the format produced by `ShellcodeWriter`), maps the shellcode into its own virtual address space, and transfers control to it. Optionally inserts a debug breakpoint opcode (`0xCC`) at the shellcode entry point.

**Dependencies**

`ShellcodeRunner` has no external dependencies.