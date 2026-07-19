/**
 * Sulfur++ C API - Native Module Interface
 * 
 * Similar to Python C API, allows writing native modules in C/C++
 * that can be imported via: import mymodule;
 * 
 * Usage:
 *   // mymodule.c
 *   #include "sulfur_api.h"
 *   
 *   static ValuePtr my_add(VM* vm, ValuePtr args, int nargs) {
 *       int64_t a = sulfur_to_int(vm, args[0]);
 *       int64_t b = sulfur_to_int(vm, args[1]);
 *       return sulfur_new_int(vm, a + b);
 *   }
 *   
 *   static SulfurMethodDef methods[] = {
 *       {"add", my_add, 2, "Add two integers"},
 *       {NULL, NULL, 0, NULL}
 *   };
 *   
 *   SULFUR_MODULE_INIT(mymodule) {
 *       sulfur_module_add_functions(mod, methods);
 *       sulfur_module_add_int_constant(mod, "VERSION", 1);
 *       return 0;
 *   }
 * 
 * Compile: gcc -shared -fPIC -o mymodule.so mymodule.c
 * Use: import mymodule; mymodule.add(2, 3)  // -> 5
 */

#ifndef SULFUR_API_H
#define SULFUR_API_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations
typedef struct SulfurVM SulfurVM;
typedef struct SulfurValue SulfurValue;
typedef struct SulfurModule SulfurModule;
typedef struct SulfurObject SulfurObject;
typedef struct SulfurList SulfurList;
typedef struct SulfurDict SulfurDict;
typedef struct SulfurString SulfurString;
typedef struct SulfurFunction SulfurFunction;

// Opaque value handle (like PyObject*)
typedef SulfurValue* ValuePtr;

// Module initialization function signature
typedef int (*SulfurModuleInitFunc)(SulfurVM* vm, SulfurModule* mod);

// Method definition (like PyMethodDef)
typedef struct {
    const char* name;           // Method name
    ValuePtr (*func)(SulfurVM* vm, ValuePtr* args, int nargs);  // C function
    int nargs;                  // Number of arguments (-1 for variable)
    const char* doc;            // Documentation string
} SulfurMethodDef;

// Module definition (like PyModuleDef)
typedef struct {
    const char* name;           // Module name
    const char* doc;            // Module documentation
    SulfurMethodDef* methods;   // Module functions
    SulfurModuleInitFunc init;  // Optional init function
} SulfurModuleDef;

// Type object (like PyTypeObject) - for custom types
typedef struct {
    const char* name;           // Type name
    const char* doc;            // Documentation
    size_t basicsize;           // Instance size
    void (*dealloc)(SulfurVM* vm, SulfurObject* obj);
    ValuePtr (*call)(SulfurVM* vm, SulfurObject* obj, ValuePtr* args, int nargs);
    ValuePtr (*getattr)(SulfurVM* vm, SulfurObject* obj, const char* name);
    int (*setattr)(SulfurVM* vm, SulfurObject* obj, const char* name, ValuePtr value);
} SulfurTypeDef;

// ==========================================
// Value Creation (constructors)
// ==========================================

ValuePtr sulfur_new_null(SulfurVM* vm);
ValuePtr sulfur_new_bool(SulfurVM* vm, int value);
ValuePtr sulfur_new_int(SulfurVM* vm, int64_t value);
ValuePtr sulfur_new_float(SulfurVM* vm, double value);
ValuePtr sulfur_new_string(SulfurVM* vm, const char* str);
ValuePtr sulfur_new_string_len(SulfurVM* vm, const char* str, size_t len);

// List operations
ValuePtr sulfur_new_list(SulfurVM* vm, size_t initial_capacity);
int sulfur_list_append(SulfurVM* vm, ValuePtr list, ValuePtr item);
int sulfur_list_extend(SulfurVM* vm, ValuePtr list, ValuePtr other_list);
ValuePtr sulfur_list_get(SulfurVM* vm, ValuePtr list, int64_t index);
int sulfur_list_set(SulfurVM* vm, ValuePtr list, int64_t index, ValuePtr value);
int64_t sulfur_list_size(SulfurVM* vm, ValuePtr list);

// Dict operations
ValuePtr sulfur_new_dict(SulfurVM* vm);
int sulfur_dict_set(SulfurVM* vm, ValuePtr dict, const char* key, ValuePtr value);
int sulfur_dict_set_value(SulfurVM* vm, ValuePtr dict, ValuePtr key, ValuePtr value);
ValuePtr sulfur_dict_get(SulfurVM* vm, ValuePtr dict, const char* key);
ValuePtr sulfur_dict_get_value(SulfurVM* vm, ValuePtr dict, ValuePtr key);
int sulfur_dict_has(SulfurVM* vm, ValuePtr dict, const char* key);
int64_t sulfur_dict_size(SulfurVM* vm, ValuePtr dict);

// Function creation
ValuePtr sulfur_new_native_function(SulfurVM* vm, const char* name, 
                                     ValuePtr (*func)(SulfurVM*, ValuePtr*, int),
                                     int nargs, const char* doc);

// Custom object creation
ValuePtr sulfur_new_object(SulfurVM* vm, SulfurTypeDef* type, void* data);

// ==========================================
// Value Inspection (type checking & conversion)
// ==========================================

int sulfur_is_null(SulfurVM* vm, ValuePtr val);
int sulfur_is_bool(SulfurVM* vm, ValuePtr val);
int sulfur_is_int(SulfurVM* vm, ValuePtr val);
int sulfur_is_float(SulfurVM* vm, ValuePtr val);
int sulfur_is_string(SulfurVM* vm, ValuePtr val);
int sulfur_is_list(SulfurVM* vm, ValuePtr val);
int sulfur_is_dict(SulfurVM* vm, ValuePtr val);
int sulfur_is_function(SulfurVM* vm, ValuePtr val);
int sulfur_is_object(SulfurVM* vm, ValuePtr val);

// Type name
const char* sulfur_type_name(SulfurVM* vm, ValuePtr val);

// Conversions
int sulfur_to_bool(SulfurVM* vm, ValuePtr val);
int64_t sulfur_to_int(SulfurVM* vm, ValuePtr val);
double sulfur_to_float(SulfurVM* vm, ValuePtr val);
const char* sulfur_to_string(SulfurVM* vm, ValuePtr val);  // Returns borrowed pointer

// Object data access
void* sulfur_object_data(SulfurVM* vm, ValuePtr obj);
SulfurTypeDef* sulfur_object_type(SulfurVM* vm, ValuePtr obj);

// ==========================================
// Module Operations
// ==========================================

// Add functions to module
void sulfur_module_add_functions(SulfurVM* vm, SulfurModule* mod, SulfurMethodDef* methods);

// Add constants to module
void sulfur_module_add_int_constant(SulfurVM* vm, SulfurModule* mod, const char* name, int64_t value);
void sulfur_module_add_float_constant(SulfurVM* vm, SulfurModule* mod, const char* name, double value);
void sulfur_module_add_string_constant(SulfurVM* vm, SulfurModule* mod, const char* name, const char* value);

// Get module by name
SulfurModule* sulfur_import_module(SulfurVM* vm, const char* name);

// ==========================================
// Function Calling
// ==========================================

// Call a Sulfur++ function from C
ValuePtr sulfur_call_function(SulfurVM* vm, ValuePtr func, ValuePtr* args, int nargs);

// Call method on object
ValuePtr sulfur_call_method(SulfurVM* vm, ValuePtr obj, const char* method, ValuePtr* args, int nargs);

// ==========================================
// Error Handling
// ==========================================

// Set exception (like PyErr_SetString)
void sulfur_set_error(SulfurVM* vm, const char* type, const char* message);

// Check if exception occurred
int sulfur_has_error(SulfurVM* vm);

// Clear exception
void sulfur_clear_error(SulfurVM* vm);

// Get exception info
const char* sulfur_error_type(SulfurVM* vm);
const char* sulfur_error_message(SulfurVM* vm);

// Predefined error types
#define SULFUR_ERROR_RUNTIME    "RuntimeError"
#define SULFUR_ERROR_TYPE       "TypeError"
#define SULFUR_ERROR_VALUE      "ValueError"
#define SULFUR_ERROR_INDEX      "IndexError"
#define SULFUR_ERROR_KEY        "KeyError"
#define SULFUR_ERROR_IO         "IOError"
#define SULFUR_ERROR_MATH       "MathError"

// ==========================================
// Memory Management
// ==========================================

// Reference counting (like Py_INCREF/Py_DECREF)
void sulfur_incref(SulfurVM* vm, ValuePtr val);
void sulfur_decref(SulfurVM* vm, ValuePtr val);

// ==========================================
// Macros for convenience
// ==========================================

#define SULFUR_RETURN_IF_ERROR(vm) \
    if (sulfur_has_error(vm)) return NULL

#define SULFUR_CHECK_ARGS(vm, args, nargs, expected) \
    if ((nargs) != (expected)) { \
        sulfur_set_error(vm, SULFUR_ERROR_TYPE, \
            "Expected " #expected " arguments, got " #nargs); \
        return NULL; \
    }

// Module initialization macro (like PyMODINIT_FUNC)
#define SULFUR_MODULE_INIT(name) \
    int sulfur_module_init_##name(SulfurVM* vm, SulfurModule* mod)

#define SULFUR_MODULE_DEF(name, doc, methods, init) \
    static SulfurModuleDef sulfur_module_def_##name = { \
        #name, doc, methods, init \
    }; \
    extern SulfurModuleDef* sulfur_get_module_def_##name(void); \
    SulfurModuleDef* sulfur_get_module_def_##name(void) { \
        return &sulfur_module_def_##name; \
    }

#ifdef __cplusplus
}
#endif

#endif // SULFUR_API_H