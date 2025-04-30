# Eraser-CD

Eraser CD is a static analysis tool designed to detect data races in C code which is designed to be implemented in a CI/CD pipeline so that only relevant changes are analysed. This tool is an approximation of the Eraser algorithm.

Eraser CD has been implemented in these three repositories:
- https://github.com/ProgrammerByte/Eraser-CD-Test-Splash-3-Barnes
- https://github.com/ProgrammerByte/Eraser-CD-Test-Splash-3-Volrend
- https://github.com/ProgrammerByte/Eraser-CD-Test-Splash-3-Ocean-Non-Contiguous

To build this project for Linux systems make sure the following packages are installed:
- cmake
- make
- build-essential
- clang
- libsqlite3-dev
- libclang-dev

Then in the root directory for the project run `cmake .` and then `make`.
