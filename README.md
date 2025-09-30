# Braindead
Braindead, is a compiler made for running Brainfuck codes. Written in C and made with compilers GCC and Clang.

## Supported In

| Linux Distros | Status   | Comment |
|---------------|----------|---------|
| Arch Linux    | ✅       | N/A     |
| Other         | ✅       | N/A     |

## Supported Commands
| Command | Description                                | Supported |
|---------|--------------------------------------------|-----------|
| `>`     | Move pointer to the right                   | ✅        |
| `<`     | Move pointer to the left                    | ✅        |
| `+`     | Increment value at pointer                  | ✅        |
| `-`     | Decrement value at pointer                  | ✅        |
| `.`     | Output character at pointer                 | ✅        |
| `,`     | Input character to pointer                  | ✅        |
| `[`     | Start loop                                  | ✅        |
| `]`     | End loop                                    | ✅        |
| `#`     | Debug print pointer and cell value         | ✅        |
| `!`     | Exit program                                | ✅        |
| `*`     | Reset pointer to start of tape             | ✅        |
| `:`     | Comment (ignored until end of line)        | ✅        |
| `;`     | Comment (ignored until end of line)        | ✅        |

## Download
You can directly download "braindead" from Github Releases page.

## What's The Commands?
Check commands by running "braindead -h".

## Errors:
"-bash: line 44: ./clang-braindead.out: Permission denied": Simple to fix, run: "chmod 777 clang-braindead.out".
