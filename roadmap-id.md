# Roadmap Sulfur++

## Fitur yang Sudah Selesai

### Bahasa Inti
- Pengetikan dinamis dengan dukungan untuk `null`, `bool`, `int_64`, `float_64`, `complex_128`, `str`, `char`, `list`, `dict`, `set`, `fn`, `class`, `struct`, `ptr`
- FFI - Interop C/C++ (`libc` `dlopen`, `dlsym`, `call`, `malloc/free`, baca/tulis memori)
- Async/await dengan coroutine berbasis stack (`ucontext`)
- Operator pipeline `|>`
- Sistem versi tunggal (`SFPP_VERSION_STRING`)
- Object.freeze() / Object.seal() (perilaku mirip JavaScript)
- REPL history dan completion (readline/editline)

### Parser & Fitur Bahasa
- Dukungan parser untuk notasi tipe pengembalian `->` dalam signature fungsi
- Deklarasi struct/class
- Ekspresi lambda dengan tipe parameter yang benar
- Ekspresi member (pencarian nama field via `structFieldNames`)
- Ekspresi new (alokasi malloc)
- Ekspresi index (indexing array/list via GEP)
- Ekspresi pipeline (`left |> right`)
- Ekspresi null coalescing (`left ?? right`)
- Literal list `[a, b, c]`
- Literal dict `{"key": value}`
- Address-of `&` dan dereference `*x`
- Ekspresi delete (`delete x`)
- Ekspresi await (async/await)
- Deklarasi interface (opaque struct)
- Statement Export/Expose/Overwrite
- Exception handling try/catch/finally (setjmp/longjmp)
- Statement throw
- Match statement (pattern matching)
- Anotasi `@jit` pada `FnDeclStmt`
- Resolusi import lengkap: relative, project-local, std/, packages/, scoped user/repo

### AOT / Kompilasi LLVM
- Penurunan (lowering) kompilasi AOT untuk semua fitur bahasa utama
- Ekspresi lambda (dengan tipe parameter yang benar)
- Resolusi modul import untuk AOT (penautan simbol modul yang diimpor)
- Optimasi AOT (eliminasi kode mati, folding konstan, inline)
- `getLLVMType` diperluas untuk tipe `str`, `list`, `any`, `fn`, `matrix`
- `emitDeclarations` dipanggil sebelum `emitAllFunctions`
- Filtering fungsi sadar `@jit` di `forwardDeclareAll` / `emitAllFunctions`
- Ekspresi panggilan FFI (`dlopen`/`dlsym` runtime binding untuk modul yang diimpor)
- Dukungan embedded (Arduino/ESP32) - LLVM AOT ke file objek
- Kompilasi silang ke board nyata (ESP-IDF, AVR-GCC, flashing)

### CLI & Tooling
- Script install/uninstall untuk Linux/macOS/Windows (dengan manajemen PATH)
- Flag CLI: `--debug`, `--lang-constants`, `--tokens`, `--parse-only`, `--ast`, `--list-modules`, `--list-builtins`
- Package manager (`sfpp -i` / `--install` / `--uninstall` / `--list-packages` / `--sync`)
- Struktur package scoped (`.sfpp/packages/<user>/<repo>/`)
- Resolusi dependensi package manager + lockfile (`sfpp-lock.toml`, `--sync`, auto-update `sfpp-project.toml`)

### Standard Library
- `std/io`: I/O sistem file dan terminal
- `std/sys`: Hardware sistem, jaringan, dan manajemen proses OS
- `std/builtin`: Builtin bahasa inti, waktu, dan utilitas matematika
- `std/collections`: Utilitas untuk list, dict, dan set
- `std/math`: Operasi matematika lanjutan dan konstruktor kompleks
- `std/matrix`: Mesin aljabar linier
- `std/string`: Manipulasi string
- `std/json`: Parsing dan serialisasi JSON

## Belum Selesai / Pekerjaan yang Tersisa

### Bahasa Inti
- Dukungan closure lengkap untuk capture lambda (capture dari scope enclosing)

### Tooling & Infrastruktur
- Verifikasi build (compile dan test semua perubahan)
- Test suite untuk fitur AOT
- Dokumentasi pengguna untuk fitur baru

### Target Embedded
- Linker script untuk target embedded ESP32/AVR (`esp32.ld`)
- Kode startup (`crt0`) untuk target embedded
- Auto-detect port serial untuk flashing
