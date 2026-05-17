
# Sulfur++ (`.sfpp`)

> Reactive systems language with modern developer ergonomics.

---

# What is Sulfur++?

Sulfur++ adalah bahasa pemrograman general-purpose modern yang:
- static-first
- interpreted
- systems-oriented
- reactive-ready
- beginner friendly
- memiliki syntax modern
- mendukung dynamic typing opsional

Sulfur++ dibuat untuk menggabungkan:
- performa dan struktur ala C++
- fleksibilitas ala Python
- kenyamanan ala JavaScript/TypeScript
- usability ala PHP

---

# Identity

| Component | Name |
|---|---|
| Language | Sulfur++ |
| Extension | `.sfpp` |
| Runtime | `combust` |
| Package Manager | `fuse` |

---

# Runtime

Runtime utama Sulfur++ bernama `combust`.

Digunakan untuk:
- menjalankan file `.sfpp`
- debugging
- hot reload
- runtime execution

## Menjalankan Program

```bash
combust main.sfpp
```

## Debug Mode

```bash
combust main.sfpp --debug
```

Debug mode digunakan untuk:
- melihat error detail
- runtime trace
- debugging internal

## Watch Mode

```bash
combust main.sfpp --watch
```

Watch mode akan:
- memonitor perubahan file
- auto restart runtime

---

# Package Manager (`fuse`)

Sulfur++ memiliki package manager bernama `fuse`.

Digunakan untuk:
- install package
- remove package
- manage dependency

## Init Project

```bash
fuse init
```

## Install Package

```bash
fuse add asep/inilibrary
```

Artinya:
- mengambil repository `inilibrary`
- dari user `asep`

## Remove Package

```bash
fuse rem asep/inilibrary
```

---

# Import System

## Basic Import

```sfpp
import @asep/mathlib as math;
```

Penjelasan:
- `@asep/mathlib` = package source
- `as math` = alias namespace

Penggunaan:

```sfpp
math.add();
```

---

# Import Flags

Sulfur++ memiliki import capability system.

Digunakan untuk:
- mengubah behavior module
- runtime capability
- namespace behavior

## Contoh

```sfpp
import @asep/mathlib as math
--use=[FULL, NOLIBNAME];
```

---

# Import Flag List

| Flag | Penjelasan |
|---|---|
| FULL | import semua public export |
| NOLIBNAME | menghapus namespace library |
| RAW | low-level/raw import |
| REACTIVE | reactive binding |
| HOTRELOAD | auto reload module |
| SAFE | strict runtime safety |
| UNSAFE | bypass safety restriction |
| TRACE | runtime tracing |
| CACHE | cache module |
| INLINE | inline optimization |

---

# Type System

Sulfur++ menggunakan static typing sebagai default.

Namun dapat memakai dynamic typing menggunakan `dyn`.

---

# Integer Types

## Signed Integer

Digunakan untuk angka positif dan negatif.

```txt
int_8
int_16
int_32
int_64
```

---

# Unsigned Integer

Hanya angka positif.

```txt
uint_8
uint_16
uint_32
uint_64
```

---

# Floating Point

Digunakan untuk angka desimal.

```txt
float_32
float_64
```

---

# Primitive Types

```txt
bool
char
str
dyn
auto
void
null
```

| Type | Penjelasan |
|---|---|
| bool | true / false |
| char | single character |
| str | string |
| dyn | runtime dynamic type |
| auto | inferred static type |
| void | tidak mengembalikan nilai |
| null | empty/null value |

---

# Collection Types

## LIST

List adalah collection berurutan.

```sfpp
list<int_32> numbers;
```

---

## SET

Set adalah collection unik tanpa duplicate.

```sfpp
set<str> usernames;
```

---

## DICT

Dictionary menyimpan key-value pair.

```sfpp
dict<str, int_32> scores;
```

---

## MATRIX

Matrix digunakan untuk data multidimensional.

```sfpp
matrix<float_32> transform;
```

---

# Nullable System

Sulfur++ mendukung nullable type.

## Nullable Type

```sfpp
str?
Player?
```

Artinya:
- value boleh null

---

# Optional Chaining

```sfpp
user?.name
```

Jika `user` null:
- tidak crash

---

# Null Coalescing

```sfpp
user ?? "Guest"
```

Jika `user` null:
- gunakan `"Guest"`

---

# Variables

## Immutable Variable

```sfpp
let username: str = "Daffa";
```

Tidak bisa diubah.

---

## Mutable Variable

```sfpp
var hp: int_32 = 100;
```

Bisa diubah.

---

## Auto Type

```sfpp
auto score = 999;
```

Type otomatis diinfer.

Tetapi tetap static type.

---

## Dynamic Type

```sfpp
dyn anything = "hello";
```

Type bisa berubah saat runtime.

---

# OOP System

Sulfur++ mendukung:
- class
- struct
- interface

---

# Class

```sfpp
class Player {

}
```

Digunakan untuk object-oriented programming.

---

# Struct

```sfpp
struct Vec2 {
    x: float_32,
    y: float_32
}
```

Struct lebih lightweight dibanding class.

---

# Interface

```sfpp
interface Drawable {
    fn draw();
}
```

Digunakan untuk contract/interface abstraction.

---

# Ordered Lifecycle Constructor System

Sulfur++ menggunakan ordered lifecycle system.

Bukan constructor tradisional biasa.

---

# Constructor Order

```sfpp
+1>loadDefaults
+2>connect
```

Artinya:
- jalankan `loadDefaults`
- lalu `connect`

---

# Destructor Order

```sfpp
~1>cleanup
```

Digunakan saat object dihancurkan.

---

# Pipeline System

Pipeline system digunakan untuk:
- chaining function
- readable processing flow

## Contoh

```sfpp
data
-> clean()
-> parse()
-> save();
```

Equivalent:

```txt
save(parse(clean(data)))
```

---

# Reactive System

Reactive system adalah salah satu core feature Sulfur++.

---

# Reactive Variable

```sfpp
reactive hp = 100;
```

Value dapat dipantau realtime.

---

# Watch Block

```sfpp
watch hp {
    print("HP changed");
}
```

Berjalan saat value berubah.

---

# Conditional Watch

```sfpp
watch hp <= 0 {
    die();
}
```

Reactive conditional system.

---

# Signal System

Digunakan untuk event system.

---

# Signal

```sfpp
signal OnDamage;
```

---

# Emit Signal

```sfpp
emit OnDamage;
```

---

# Listen Signal

```sfpp
on OnDamage {
    shakeScreen();
}
```

---

# Property System

Property system digunakan untuk:
- runtime metadata
- runtime behavior
- object capability

---

# withProperties

```sfpp
player.withProperties([
    PROP_FROZEN,
    PROP_READONLY
]);
```

---

# Property Constants

## Core Properties

```txt
PROP_FROZEN
PROP_READONLY
PROP_IMMUTABLE
PROP_REACTIVE
PROP_SERIALIZABLE
PROP_UNSAFE
```

## Extended Properties

```txt
PROP_VOLATILE
PROP_DEBUG
PROP_HIDDEN
PROP_PRIVATE
PROP_PUBLIC
PROP_FINAL
PROP_CONST
PROP_STATIC
PROP_SYNC
PROP_ASYNC
```

## Advanced Properties

```txt
PROP_LAZY
PROP_CACHED
PROP_TEMP
PROP_NATIVE
PROP_PROTECTED
PROP_INTERNAL
PROP_EXPERIMENTAL
PROP_DEPRECATED
PROP_LOCKED
PROP_OBSERVABLE
```

---

# Periodic String System (`ps""`)

Periodic String adalah advanced string system.

Terinspirasi dari:
- Python f-string
- JS template literal

Tetapi lebih powerful.

---

# Basic Interpolation

```sfpp
ps"Hello {name}"
```

---

# Expression Evaluation

```sfpp
ps"{2 + 2}"
```

---

# Sequence Formatting

```sfpp
ps"{'-':repeat(10)}"
```

---

# Join Formatting

```sfpp
ps"{nums:join(', ')}"
```

---

# Safe Nullable Formatting

```sfpp
ps"{user?.name ?? 'Guest'}"
```

---

# Pipeline Formatting

```sfpp
ps"{name -> upper() -> trim()}"
```

---

# Streams

Sulfur++ memiliki high-level stream API.

---

# High-Level Stream

```sfpp
Terminal.Out << "Hello";
Terminal.Warn << "Warning";
Terminal.Err << "Error";
```

---

# Input Stream

```sfpp
let name = Terminal.In.read();
```

---

# Low-Level Stream

```txt
stdout
stdin
stderr
stdwarn
```

---

# Memory & Safety

Sulfur++ tetap mendukung low-level capability.

---

# Pointer

```sfpp
ptr<int_32> p = &x;
```

---

# Unsafe Block

```sfpp
unsafe {

}
```

Digunakan untuk bypass safety system.

---

# Defer

```sfpp
defer {
    file.close();
}
```

Akan berjalan otomatis saat keluar scope.

---

# Constants

Sulfur++ memiliki banyak built-in constants.

---

# Math Constants

```txt
PI
E
INF
NEG_INF
NAN
TAU
PHI
```

---

# Runtime Constants

```txt
RUNTIME_VERSION
LANG_VERSION
OS
ARCH
BUILD_MODE
DEBUG_MODE
```

---

# Time Constants

```txt
SECOND
MINUTE
HOUR
DAY
WEEK
MONTH
YEAR
```

---

# Error System

Format:

```txt
<SEVERITY>_<CATEGORY>_<CODE>
```

---

# Severity

```txt
FE
E
W
```

| Severity | Penjelasan |
|---|---|
| FE | fatal error |
| E | normal error |
| W | warning |

---

# Example Error

```txt
E_TYPE_001
E_IMPORT_409
FE_VM_001
W_UNUSED_001
```

---

# Tools

| Tool | Fungsi |
|---|---|
| combust | runtime/interpreter |
| fuse | package manager |
| sfmt | formatter |
| sflint | linter |

---

# Sulfur++ Signature Features

- ordered lifecycle constructor system
- reactive watch system
- pipeline syntax
- periodic string system
- import capability system
- runtime property system
- static-first typing
- optional dynamic typing
- modern readable systems syntax
