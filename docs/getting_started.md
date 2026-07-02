# Getting Started

## Build the Project
```bash
git clone https://github.com/yourorg/sulfurplusplus-lang.git
cd sulfurplusplus-lang
mkdir build && cd build
cmake .. && cmake --build . --config Release
```

## Run the REPL
```bash
./combust   # starts interactive prompt
```

## Execute a Script
Create `hello.sfpp`:
```sfpp
import std/io as io;
io.Terminal.Out << "Hello, Sulfur++!" << "\n";
```
Run it:
```bash
./combust hello.sfpp
```

## Project Layout
- `src/` – core implementation (lexer, parser, interpreter).
- `include/` – public headers (`token.hpp`, etc.).
- `examples/` – sample scripts.
- `src/stdlib/` – standard library modules.
- `docs/` – this documentation folder.

---
For deeper language concepts, see the next tutorial sections.
