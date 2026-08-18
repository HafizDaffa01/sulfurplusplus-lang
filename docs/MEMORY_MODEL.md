# Spesifikasi Resmi Model Memori Sulfur++ (Sulfur++ Memory Model Specification)

Dokumen ini mendefinisikan arsitektur resmi pengelolaan memori, representasi nilai, masa hidup objek (*object lifetime*), kepemilikan (*ownership*), dan batasan keamanan (*unsafe boundary*) untuk bahasa pemrograman **Sulfur++**.

---

## 1. Klasifikasi Tipe: Value Types vs Reference Types

Sulfur++ membedakan representasi data dalam memori menjadi dua kategori utama:

```
┌─────────────────────────────────────────────────────────────┐
│                   Sulfur++ Type System                      │
└──────────────────────────────┬──────────────────────────────┘
                               │
        ┌──────────────────────┴──────────────────────┐
        ▼                                             ▼
  Value Types (Stack)                       Reference Types (Heap)
  ───────────────────                       ──────────────────────
  • bool                                    • str
  • int_64                                  • list
  • float_64                                • dict
  • char                                    • set
  • complex_128                             • class instances
  • struct instances                        • fn / closures
```

### A. Value Types (Semantik Nilai / Stack)
* **Tipe**: `bool`, `int_64`, `float_64`, `char`, `complex_128`, dan `struct`.
* **Alokasi**: Disimpan langsung di Call Stack (atau register CPU pada kompilasi native/LLVM).
* **Perilaku Assignment (`=`)**: *Copy-by-value* (duplikasi nilai penuh).
* **Contoh**:
  ```sfpp
  var a = 10;
  var b = a;    // b adalah salinan independen dari a
  b = 20;       // a tetap bernilai 10
  ```

### B. Reference Types (Semantik Referensi / Heap)
* **Tipe**: `str`, `list`, `dict`, `set`, `class instances`, `fn` (closures).
* **Alokasi**: Data aktual dialokasikan di Heap, sedangkan variabel di stack hanya menyimpan *pointer/handle* cerdas.
* **Perilaku Assignment (`=`)**: *Reference alias* (berbagi objek yang sama).
* **Jawaban Pertanyaan Kasus `let b = a`**:
  ```sfpp
  let a = [1, 2, 3];
  let b = a;        // a dan b MENUNJUK objek list yang sama di Heap!
  b[0] = 99;        // a[0] juga berubah menjadi 99
  ```
* Untuk membuat salinan independen, gunakan fungsi kloning eksplisit:
  ```sfpp
  let b = a.clone(); // Alokasi objek list baru di heap
  ```

---

## 2. Object Lifetime & Scope Escape Safety

Dalam Sulfur++, objek di Heap dikelola dengan **Deterministic Automatic Reference Counting (ARC)**:

```
Scope: create()
┌───────────────────────┐
│ let data = [1, 2, 3]  │ ──► [Heap Object: List [1,2,3] | RefCount = 1]
│ return data           │ ──► Caller menerima referensi (RefCount = 2)
└───────────────────────┘
  (Scope Exit: `data` destroyed, RefCount = 1)
```

### Evaluasi Kasus:
```sfpp
fn create() {
    let data = [1, 2, 3];
    return data;
}

let result = create();
```
* **Mekanisme**: Ketika `create()` mengembalikan `data`, *reference count* objek list dipertahankan saat diserahkan ke variabel pemanggil `result`.
* **Keamanan**: Bebas dari *dangling pointer* dan *use-after-free*. Objek hanya akan didealokasi dari heap ketika `RefCount == 0`.

---

## 3. Perbedaan Arsitektur: `struct` vs `class`

| Karakteristik | `struct` | `class` |
| :--- | :--- | :--- |
| **Lokasi Memori** | Stack (inline contiguous memory) | Heap (pointer reference) |
| **Semantik Penugasan** | *Value Copy* (salinan independen) | *Reference Sharing* |
| **Metode & Lifecycle** | Data murni (tanpa ctor/dtor order) | Mendukung `+N>init` dan `~N>cleanup` |
| **Overhead Alokasi** | Nol (zero heap allocation) | Alokasi heap dengan ARC |

---

## 4. Siklus Hidup Deterministik (RAII, `defer`, dan `delete`)

Sulfur++ menerapkan pembersihan memori deterministik berbasis RAII:

1. **Urutan Konstruktor & Destruktor**:
   * Konstruktor dieksekusi secara berurutan sesuai prioritas (`+1>initA`, `+2>initB`).
   * Destruktor dieksekusi secara terbalik (*reverse order*: `~2>cleanupB`, `~1>cleanupA`).
2. **Statement `defer`**:
   * Mengeksekusi blok kode tepat saat scope fungsi/blok berakhir (urutan LIFO).
3. **Statement `delete`**:
   * Memungkinkan pelepasan resource secara eksplisit dan segera memicu pemanggilan destruktor objek.

---

## 5. Pointer Semantics & Batasan `unsafe`

Sulfur++ menyediakan akses tingkat rendah melalui tipe `ptr` dengan batasan keamanan yang ketat:

```sfpp
var x = 100;

unsafe {
    var p = &x;     // Mengambil alamat memori dari x
    *p = 250;       // Mutasi langsung memori melalui dereference
}
```

### Aturan Kepemilikan & Tanggung Jawab Memori pada `unsafe`:
1. **Pointer Lokal (`&x`)**:
   * Valid selama variabel `x` masih hidup dalam scope aktif.
2. **Pointer Alokasi Manual (FFI / `libc.malloc`)**:
   * Pemrogram bertanggung jawab penuh untuk membebaskan memori melalui `libc.free(p)` atau destruktor pembungkus RAII.
3. **Semantic Boundary**:
   * Operasi dereference `*p` atau manipulasi alamat mentah di luar blok `unsafe` ditolak pada tahap **Semantic Analysis**.

---

## 6. Ringkasan Matriks Model Memori

```
┌─────────────────┬──────────────┬──────────────────┬────────────────────────┐
│ Tipe Data       │ Alokasi      │ Manajemen        │ Semantik Assignment    │
├─────────────────┼──────────────┼──────────────────┼────────────────────────┤
│ int/float/bool  │ Stack        │ Otomatis (Stack) │ Value Copy             │
│ struct          │ Stack        │ Otomatis (Stack) │ Deep Field Copy        │
│ class           │ Heap         │ ARC + RAII       │ Reference Share        │
│ list/dict/set   │ Heap         │ ARC              │ Reference Share        │
│ str             │ Heap         │ ARC              │ Immutable Reference    │
│ ptr (unsafe)    │ Register/Raw │ Manual           │ Direct Memory Address  │
└─────────────────┴──────────────┴──────────────────┴────────────────────────┘
```
