# kit-OS-2023
Kit untuk IF2230 - Sistem Operasi 2023

# Progress
## Milestone 1 (100%)
1. Done build script.
2. Done GDT, Config, etc.

## Milestone 2 
1. Done interrupts
2. Done keyboard
3. Done crud filesys 

## MILESTONE 3
1. ls -- Done
2. cd -- Done
3. mkdir -- Done (No file the same as cwd folder)
4. whereis -- Done 
5. cat -- Done (cat file up to 4 frame buffer)
6. rm -- Done  (delete file, empty folder0
7. cp -- Done (First argument is file)
8. mv -- Done (First argument is file)

# How to run
1. Be on root directory, run `make disk` to generate new disk.
2. Run `make build`.
3. Run `make insert-shell`
4. To execute OS iso, run `make run`

Alternatively, run `make all`. This will delete all disk and make everything new from scratch. If you want to save the disk/files you have created, run `make run`

