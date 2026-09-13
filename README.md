# Custom POSIX-Compliant C Shell

A lightweight, POSIX-compliant system shell built entirely from scratch in C. This project is a practical exercise in understanding UNIX system mechanics, process management, and low-level I/O manipulation.

## Features

* **I/O Redirection:** Handles standard output and error redirection for overwrite (`>`, `1>`, `2>`) and append (`>>`, `2>>`) modes.
* **Process Execution:** Resolves external programs using the `PATH` environment variable and manages child processes.
**Built-in Commands:** Includes basic shell commands (`cd`, `pwd`, `echo`, `type`, `exit`), along with `HOME` variable parsing and `~` expansion for directory navigation.
* **Input Parsing:** Uses a custom state-machine tokenizer to handle single quotes (`'`), double quotes (`"`), and backslash escaping (`\`) in-place.