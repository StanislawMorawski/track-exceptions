# Tool and include header for tracking C++ Exceptions

This is a simple POC tool for tracking which exceptions are thrown, and ensuring proper annotation a Java style.

C++ Exceptions have countless issues, from slow performance to readability.
While generally I would recommend avoid using exceptions and instead switch to more modern std::expected, this
tool showcases how we could improve readability of exceptions, while sticking to c++ standard.

## Concept

C++ standard has very relaxed rules regarding attributes, and just about any identifier, possibly with argument
list can be used. As such we can make use of clang::annotate attribute to provide additional information to
code analysis tools. For convenience sake, installing this tool, installs "annotate_throw.hpp" header file containing THROWS(...) macro, whose arguments are types that can be thrown by the function.

### Examples

#### Declaring that function can throw specific type
In examples below, we are declaring that function f may throw an 'int' type.
This annotation is not required, since int is a primitive type and hance could not be annotated with
"ALWAYS_TRACK" macro. However because function f annotates that it throws an int, each and every single function
calling f(), (and not catching the integer, or re-throwing it), must also annotate that it can throw int.
``` c++
void f [[THROWS(int)]] () {
    throw 0;
}
```
``` c++
[[THROWS(int)]] void f () {
    throw 0;
}
```

#### Declaring that function can throw several types
``` c++
[[THROWS(int, double)]] void f (bool a) {
    if(a) throw 0;
    throw 1.;
}
```

#### Declaring that every time an object of a class is thrown, it has to be annotated
``` c++
struct [[ALWAYS_TRACK]] Error {
};

```
#### All declarations of a function (or a method) must have matching annotations
If there is a mismatch, the tool will print a warning
``` c++
[[THROWS(int)]] void f ();

[[THROWS(int)]] void f () {
    throw 0;
}
```


## USAGE
track-exceptions [options] \<source0> [... \<sourceN>]

OPTIONS:

Generic Options:

  --help                      - Display available options (--help-hidden for more) \
  --help-list                 - Display list of available options (--help-list-hidden for more) \
  --version                   - Display the version of this program

track-exceptions options:

  --extra-arg=\<string>        - Additional argument to append to the compiler command line \
  --extra-arg-before=\<string> - Additional argument to prepend to the compiler command line \
  -p \<string>                 - Build path

-p \<build-path> is used to read a compile command database.

  For example, it can be a CMake build directory in which a file named
  compile_commands.json exists (use -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
  CMake option to get this output). When no build path is specified,
  a search for compile_commands.json will be attempted through all
  parent paths of the first input file . See:
  https://clang.llvm.org/docs/HowToSetupToolingForLLVM.html for an
  example of setting up Clang Tooling on a source tree.

\<source0> ... specify the paths of source files. These paths are
    looked up in the compile command database. If the path of a file is
    absolute, it needs to point into CMake's source tree. If the path is
    relative, the current working directory needs to be in the CMake
    source tree and the file must be in a subdirectory of the current
    working directory. "./" prefixes in the relative files will be
    automatically removed, but the rest of a relative path must be a
    suffix of a path in the compile command database.

## Design principles

### Limitations
- It has to be compatible with stl, and all 3rd party libraries.
- Due to technical limitations ool has to be run on a single source file at a time.\
As such it can only analyze it and all included headers, and cannot recursively analyze function calls.
- function pointers don't work and exceptions thrown within functions called using a function pointer cannot
be tracked.

### Solutions
- All function declarations must include throw annotation. Thanks to that tool can know whether and which
tracked exceptions can be throw due to specific function call
- Unless explicitly specified no exceptions are tracked by default. as STL alone throws exceptions in countless
places, many not in header files, it is not possible to know all exceptions that could be thrown. So the tool is
limited to tracking exceptions which were previously annotated to by thrown by one of the functions that are called
within analyzed function, or throwing exceptions of type which was annotated with ALWAYS_TRACK. This behavior is
in fact quite similar to one in Java, where all user exceptions and some standard exceptions are always being
tracked, but some standard exceptions, like NullPointerException are not.
- 