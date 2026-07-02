# Overview

Welcome to **Sulfur++**!

- **What?** A C++‑backed scripting language for desktop, embedded, and IoT.
- **Key goals**: fast execution, dynamic typing via `auto`, easy system integration.
- **Getting started**:
  ```bash
  # Clone the repo
  git clone https://github.com/yourorg/sulfurplusplus-lang.git
  cd sulfurplusplus-lang
  mkdir build && cd build
  cmake .. && cmake --build . --config Release
  # Run REPL
  ./combust
  ```
- **First program** (`hello.sfpp`):
  ```sfpp
  import std/io as io;
  io.Terminal.Out << "Hello, Sulfur++!" << "\n";
  ```
  Run with `combust hello.sfpp`.

For deeper tutorials, see the other docs in this folder.
