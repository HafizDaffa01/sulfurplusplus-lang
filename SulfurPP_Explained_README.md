# Sulfur++ (`.sfpp`)

> Reactive systems language with modern developer ergonomics.

---

# What is Sulfur++?

Sulfur++ is a modern general-purpose programming language that is:
- static-first
- interpreted
- systems-oriented
- reactive-ready
- beginner-friendly
- possesses modern syntax
- supports optional dynamic typing

Sulfur++ is built to combine:
- performance and structure like C++
- flexibility like Python
- ergonomics like JavaScript/TypeScript
- usability like PHP

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

The main runtime of Sulfur++ is named `combust`.

Used for:
- running `.sfpp` files
- debugging
- hot reload
- runtime execution

## Executing a Program

```bash
combust main.sfpp
```

## Debug Mode

```bash
combust main.sfpp --debug
```

Debug mode is used to:
- view detailed errors
- trace runtime execution
- perform internal debugging

## Watch Mode

```bash
combust main.sfpp --watch
```

Watch mode will:
- monitor file changes
- auto-restart the runtime

---

# Package Manager (`fuse`)

Sulfur++ has a package manager named `fuse`.

Used to:
- install packages
- remove packages
- manage dependencies

## Init Project

```bash
fuse init
```

## Install Package

```bash
fuse add asep/inilibrary
```

Meaning:
- fetches the `inilibrary` repository
- from the user `asep`

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

Explanation:
- `@asep/mathlib` = package source
- `as math` = alias namespace

Usage:

```sfpp
math.add();
```

---

# Import Flags

Sulfur++ has an import capability system.

Used to:
- modify module behavior
- manage runtime capabilities
- define namespace behavior

## Example

```sfpp
import @asep/mathlib as math
--use=[FULL, NOLIBNAME];
```

---

# Import Flag List

| Flag | Explanation |
|---|---|
| FULL | import all public exports |
| NOLIBNAME | remove the library namespace |
| RAW | low-level/raw import |
| REACTIVE | reactive binding |
| HOTRELOAD | auto-reload module |
| SAFE | strict runtime safety |
| UNSAFE | bypass safety restrictions |
| TRACE | runtime tracing |
| CACHE | cache module |
| INLINE | inline optimization |

---

# Type System

Sulfur++ uses static typing by default.

However, you can use dynamic typing using `dyn`.

---

# Integer Types

## Signed Integer

Used for positive and negative numbers.

```txt
int_8
int_16
int_32
int_64
```

---

# Unsigned Integer

Positive numbers only.

```txt
uint_8
uint_16
uint_32
uint_64
```

---

# Floating Point

Used for decimal numbers.

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

| Type | Explanation |
|---|---|
| bool | true / false |
| char | single character |
| str | string |
| dyn | runtime dynamic type |
| auto | inferred static type |
| void | does not return a value |
| null | empty/null value |

---

# Collection Types

## LIST

List is an ordered collection.

```sfpp
list<int_32> numbers;
```

---

## SET

Set is a collection of unique elements without duplicates.

```sfpp
set<str> usernames;
```

---

## DICT

Dictionary stores key-value pairs.

```sfpp
dict<str, int_32> scores;
```

---

## MATRIX

Matrix is used for multidimensional data.

```sfpp
matrix<float_32> transform;
```

---

# Nullable System

Sulfur++ supports nullable types.

## Nullable Type

```sfpp
str?
Player?
```

Meaning:
- the value can be null

---

# Optional Chaining

```sfpp
user?.name
```

If `user` is null:
- does not crash

---

# Null Coalescing

```sfpp
user ?? "Guest"
```

If `user` is null:
- falls back to `"Guest"`

---

# Variables

## Immutable Variable

```sfpp
let username: str = "Daffa";
```

Cannot be modified.

---

## Mutable Variable

```sfpp
var hp: int_32 = 100;
```

Can be modified.

---

## Auto Type

```sfpp
auto score = 999;
```

Type is automatically inferred.

But remains a static type.

---

## Dynamic Type

```sfpp
dyn anything = "hello";
```

Type can change at runtime.

---

# OOP System

Sulfur++ supports:
- class
- struct
- interface

---

# Class

```sfpp
class Player {

}
```

Used for object-oriented programming.

---

# Struct

```sfpp
struct Vec2 {
    x: float_32,
    y: float_32
}
```

Struct is more lightweight than class.

---

# Interface

```sfpp
interface Drawable {
    fn draw();
}
```

Used for contract/interface abstraction.

---

# Ordered Lifecycle Constructor System

Sulfur++ uses an ordered lifecycle system.

Rather than typical traditional constructors.

---

# Constructor Order

```sfpp
+1>loadDefaults
+2>connect
```

Meaning:
- run `loadDefaults`
- then `connect`

---

# Destructor Order

```sfpp
~1>cleanup
```

Invoked when the object is destroyed.

---

# Pipeline System

Pipeline system is used for:
- chaining functions
- creating a readable data processing flow

## Example

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

The reactive watch system is one of the core features of Sulfur++.

---

# Reactive Variable

```sfpp
reactive hp = 100;
```

The value can be monitored in real-time.

---

# Watch Block

```sfpp
watch hp {
    print("HP changed");
}
```

Executes automatically when the value changes.

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

Used for the event system.

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

The property system is used for:
- runtime metadata
- runtime behavior configuration
- object capabilities

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
^^PROP_OBSERVABLE
```

---

# Periodic String System (`ps""`)

Periodic String is an advanced string interpolation system.

Inspired by:
- Python f-string
- JS template literal

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

Sulfur++ possesses a high-level stream API.

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

# Member Access & Scope Operators

| Operator | Used for | Example | Needs object? |
|---|---|---|---|
| `.` | Regular objects (non-pointer) | `obj.name` | ✅ Yes (instance) |
| `->` | Pointer to objects | `ptr->name` | ✅ Yes (instance via pointer) |
| `::` | Class/namespace (static / Scope Resolution Operator - SRO) | `Class::staticVar` | ❌ No |

---

# Memory & Safety

Sulfur++ continues to support low-level system capabilities.

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

Used to bypass the safety system bounds.

---

# Defer

```sfpp
defer {
    file.close();
}
```

Runs automatically when exiting the scope.

---

# Constants

Sulfur++ has many built-in constants.

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

| Severity | Explanation |
|---|---|
| FE | fatal error |
| E | normal error |
| W | warning |

---

# Example Error

```txt
E_TYPE_406
E_IMPORT_409
FE_VM_500
E_LEX_400
E_NAME_404
E_PARSE_400
```

---

# Tools

| Tool | Function |
|---|---|
| combust | runtime/interpreter |
| fuse | package manager |
| sfmt | code formatter |
| sflint | code linter |

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
