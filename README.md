# kit-OS-2023: IF2230 - Operating Systems Project

A comprehensive kit for **IF2230 - Operating Systems 2023**, focusing on building an x86 Operating System with multiple milestones.

---

## 🛠 Project Overview

**Project Name:** HolOS  
**Objective:** Development of an x86 Operating System in multiple stages:  
1. Booting and Protected Mode  
2. Interrupts, Drivers, and Filesystems  
3. Shell, Paging, and User Mode  

Each milestone involves building fundamental operating system components, progressing from basic kernel setup to a functional user-mode shell.

---

## 🚀 Progress

### **Milestone 1: Booting, Kernel, and Protected Mode (100%)**
- [x] Completed Build Script Automation
- [x] Configured GDT and Protected Mode Setup
- [x] Simple Text Output with Framebuffer
- [x] Basic Kernel Development in C

### **Milestone 2: Interrupts, Drivers, and Filesystem**
- [x] Implemented Interrupt Descriptor Table (IDT)
- [x] Developed Keyboard and Disk Drivers
- [x] Created Custom Filesystem (FAT32 - IF2230 Edition)
- [x] Supported Basic Filesystem Operations: CRUD

### **Milestone 3: Shell, Paging, and User Mode**
- [x] Developed Paging Mechanism and Higher Half Kernel
- [x] Established User Mode and GDT for Privilege Levels
- [x] Implemented System Calls for Shell Commands
- [x] Built-In Shell Commands:
  - [x] `ls` - List Directory Contents
  - [x] `cd` - Change Directory
  - [x] `mkdir` - Create Directory
  - [x] `whereis` - Search Filesystem
  - [x] `cat` - Read File Content
  - [x] `rm` - Remove File/Folder
  - [x] `cp` - Copy File
  - [x] `mv` - Move/Rename File/Folder

---

## ⚙️ How to Run

1. Clone the repository and navigate to the root directory.
2. Generate a new disk image: `make disk`
3. Build the OS: `make build`
4. Insert the shell program into the filesystem: `make insert-shell`
5. Run the OS in QEMU: `make run`
6. Alternatively, you can run everything from scratch: `make all`

Note: Using make all will erase any existing disk/files. If you want to preserve your disk and files, run make run only.

Thanks to the course assistant team for the support!
