# Spesifikasi Bahasa Pemrograman Sulfur++ (Sulfur++ Language Specification)
**Versi Spesifikasi:** 1.0 (Stabil)  
**Status:** Resmi (Official Specification)  
**Filosofi:** *"Write Easy, Perform Fast"* — Menggabungkan kemudahan sintaksis modern tingkat tinggi dengan presisi dan performa sistem native C++.

---

## 1. Variabel dan Deklarasi (Variable Syntax)

Sulfur++ membedakan deklarasi variabel berdasarkan mutabilitas dan inferensi tipe:

| Keyword | Mutabilitas | Keterangan |
| :--- | :--- | :--- |
| `var` | **Mutable** | Variabel dapat diubah/di-reassign kapan saja. |
| `let` | **Immutable** | Konstanta; nilai tidak dapat diubah setelah diinisialisasi. Pelanggaran dideteksi pada tahap *Semantic Resolver*. |
| `auto` | **Mutable (Inferred)** | Kompatibilitas untuk variabel dinamis yang tipe awalnya ditentukan dari ekspresi inisialisasi. |

### Sintaksis:
```sfpp
var nama_variabel = ekspresi;
let konstanta = ekspresi;
var nama_variabel: tipe = ekspresi;  // Optional type annotation
```

### Contoh:
```sfpp
var counter = 0;
counter = counter + 1; // Sah

let PI = 3.1415926535;
// PI = 3.0;           // ERROR: [E_SEMANTIC_400] Cannot reassign to immutable variable 'PI' declared with 'let'

var usia: int_64 = 25;
```

---

## 2. Fungsi dan Penutupan (Function & Closures)

Fungsi di Sulfur++ adalah *first-class citizens* yang dapat disimpan dalam variabel, dilewatkan sebagai parameter, dan dikembalikan sebagai *closure*.

### A. Deklarasi Fungsi Bernama:
```sfpp
fn namaFungsi(param1, param2) {
    return param1 + param2;
}

// Dengan Type Annotation (Opsional)
fn tambah(a: int_64, b: int_64): int_64 {
    return a + b;
}
```

### B. Anonymous Functions & Lambdas:
```sfpp
var kaliDua = fn(x) { return x * 2; };
var kuadrat = fn(x: float_64): float_64 { return x * x; };
```

### C. Closures (Lexical Scoping Capture):
```sfpp
fn buatPenghitung(awal) {
    var hitungan = awal;
    return fn() {
        hitungan = hitungan + 1;
        return hitungan;
    };
}

var c = buatPenghitung(10);
c(); // 11
c(); // 12
```

---

## 3. Kontrol Alur (Control Flow)

### A. Kondisional `if / else if / else`:
```sfpp
if (kondisi1) {
    // Blok dieksekusi jika kondisi1 bernilai truthy
} else if (kondisi2) {
    // Blok alternatif
} else {
    // Default fallback
}
```

### B. Pencocokan Pola (`match` Expression):
```sfpp
match (ekspresi) {
    pola1 => { /* aksi 1 */ }
    pola2 => { /* aksi 2 */ }
    _     => { /* wildcard default */ }
}
```

### C. Operator Ternary & Null-Coalescing:
```sfpp
var status = (usia >= 18) ? "Dewasa" : "Anak-anak";
var hasil = nilaiMungkinNull ?? "Nilai Default";
```

---

## 4. Struktur Perulangan (Loop Syntax)

Sulfur++ mendukung dua jenis loop utama serta perulangan berbasis kondisi:

### A. `while` Loop:
```sfpp
while (kondisi) {
    // Iterasi selama kondisi == true
}
```

### B. Iterasi Koleksi (`for ... in`):
```sfpp
for (item in koleksi) {
    // Mengiterasi elemen list, karakter string, atau set
}
```

### C. C-Style `for` Loop:
```sfpp
for (var i = 0; i < 10; i = i + 1) {
    if (i == 3) continue; // Lewati iterasi
    if (i == 8) break;    // Hentikan loop
}
```

---

## 5. Sistem Tipe (Type System)

Sulfur++ mengadopsi sistem tipe **Gradual & Dynamic Typing** dengan backend C++ native yang ketat:

### Tipe Primitif:
* `null`: Nilai kosong atau tidak terdefinisi.
* `bool`: Boolean literal `true` atau `false`.
* `int_64`: Bilangan bulat 64-bit bertanda (`int8`, `int16`, `int32`, `int64`, `uint8`, dll didukung).
* `float_64`: Bilangan desimal presisi ganda 64-bit IEEE 754.
* `complex_128`: Bilangan kompleks presisi tinggi ($a + bi$).
* `str`: String karakter UTF-8.
* `char`: Karakter ASCII tunggal (`'A'`).

### Tipe Koleksi & Struktur:
* `list`: Array dinamis (`[1, "dua", 3.0]`), dapat membentuk matriks multi-dimensi.
* `dict`: Hash map berbasis string key (`{"kunci": "nilai"}`).
* `set`: Himpunan elemen unik.
* `fn`: Objek fungsi/closure.
* `class` / `struct`: Tipe objek kustom.
* `ptr`: Pointer alamat memori mentah (*Unsafe mode*).

---

## 6. Cakupan dan Variabel (Scope & Resolution)

1. **Lexical Block Scope**: Setiap blok kode dalam `{ ... }` memiliki lingkungan leksikal terisolasi.
2. **Variable Shadowing**: Variabel di dalam inner scope dapat membayangi (*shadow*) variabel di outer scope tanpa merusak nilai aslinya di luar.
3. **Immutability Enforcement**: Variabel yang dideklarasikan dengan `let` akan divalidasi oleh *Resolver* pada pass kompilasi statis. Re-assignment akan langsung menggagalkan eksekusi.
4. **Keyword Isolation**: Penggunaan `break`, `continue`, dan `this` di luar konteks yang valid ditolak sebelum runtime.

---

## 7. Pemrograman Berorientasi Objek (Class & Struct)

### A. Kelas (`class`):
Kelas di Sulfur++ adalah tipe referensi dengan siklus hidup (*lifecycle*) bertingkat:
* `+N>init`: Prioritas eksekusi konstruktor (angka lebih kecil dijalankan lebih dulu).
* `~N>cleanup`: Prioritas eksekusi destruktor saat objek dihapus atau keluar scope.
* `this` / `self`: Referensi instans saat ini.

```sfpp
class RekeningBank {
    pemilik;
    saldo;

    +1>init;
    fn init(nama, saldoAwal) {
        this.pemilik = nama;
        this.saldo = saldoAwal;
    }

    fn setor(jumlah) {
        this.saldo = this.saldo + jumlah;
        return this.saldo;
    }

    ~1>cleanup;
    fn cleanup() {
        // Pembersihan otomatis saat objek dihancurkan
    }
}

var acc = new RekeningBank("Alice", 1000);
acc.setor(500);
```

### B. Struktur Data (`struct`):
Tipe komposit ringan untuk representasi *Plain Old Data* (POD):
```sfpp
struct Titik2D {
    x: float_64,
    y: float_64
}

var p = Titik2D{x: 10.5, y: 20.0};
```

---

## 8. Sistem Modul & Ekspor (Module System)

### A. Mengimpor Modul:
```sfpp
import std/io as io;
import std/crypto as crypto;
import std/regex as regex;
```

### B. Mendefinisikan Modul:
```sfpp
// mymodule.sfpp
export this as my/module;

fn utilitas() {
    return "OK";
}
```

### C. Mengekspos Fungsi Native (C++ Bridge):
```sfpp
expose "crypto_sha256" as sha256;
```

---

## 9. Penanganan Error (Error Handling)

Sulfur++ menggunakan model penanganan eksepsi terstruktur `try / catch / finally` dengan kamus informasi error:

```sfpp
try {
    if (divisor == 0) {
        throw "Tidak dapat membagi dengan nol";
    }
} catch (e) {
    // 'e' adalah Dictionary berisi metadata error:
    // e["message"] : Pesan kesalahan
    // e["code"]    : Kode kesalahan (misal: E_RUNTIME_500, E_TYPE_406)
    // e["line"]    : Baris tempat terjadinya error
    // e["hint"]    : Saran perbaikan jika tersedia
    io.Terminal.Out << "Error [" << e["code"] << "]: " << e["message"] << "\n";
} finally {
    // Blok yang selalu dieksekusi baik terjadi error maupun tidak
}
```

### Defer Statement (`defer`):
Menjadwalkan eksekusi pembersihan tepat saat blok/fungsi saat ini keluar (LIFO order):
```sfpp
fn prosesFile() {
    var f = io.readFile("data.txt");
    defer {
        io.Terminal.Out << "Pembersihan file selesai.\n";
    }
    // Lakukan pemrosesan...
}
```

---

## 10. Manajemen Memori (Memory Management)

1. **Automatic Reference Counting (ARC) & RAII**:
   * Objek bahasa tingkat tinggi (`str`, `list`, `dict`, `class instance`) dikelola secara otomatis menggunakan smart pointer C++ (`std::shared_ptr` dengan semantic copy/move efisien).
2. **Explicit Deallocation (`delete`)**:
   * Keyword `delete varName;` secara eksplisit memicu pemanggilan destruktor terurut (`~N>`) dan melepaskan referensi memori seketika.
3. **No Garbage Collection Stalls**:
   * Karena menggunakan RAII deterministik, tidak ada jeda *Stop-The-World* (GC pause), menjadikannya ideal untuk IoT dan pemrosesan real-time.

---

## 11. Unsafe & Operasi Pointer (Unsafe & Pointers)

Untuk keperluan interoperabilitas hardware, IoT, dan manipulasi memori tingkat rendah, Sulfur++ menyediakan blok `unsafe`:

### Aturan Keamanan:
* Kata kunci `unsafe` hanya dapat digunakan di dalam modul library atau dengan hak akses sistem.
* Di dalam blok `unsafe`, operasi pointer diaktifkan:
  * `&x`: Mengambil referensi pointer dari variabel.
  * `*ptr`: Melakukan dereferensi pointer untuk membaca/menulis nilai.
  * `ptr->member`: Mengakses member dari objek yang ditunjuk oleh pointer.

```sfpp
// Contoh manipulasi pointer
unsafe {
    var angka = 42;
    var p = &angka;    // p bertipe ptr
    *p = 100;          // Mengubah nilai asli 'angka' secara langsung melalui memori
}
```
