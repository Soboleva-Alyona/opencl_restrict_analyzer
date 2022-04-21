# OpenCL Static Analyzer

The analyzer is focused on detecting issues related to memory accesses.
It is based on parsing the program using Clang and converting into a formal representation that gets fed to the Z3 solver.

## Using the Command Line Tool

You shall pass the path to the file with the OpenCL code (`--input`), the name of the kernel function (`--kernel`) and its arguments to the tool, as well as specify the checks to perform.
Currently there are two checks available: `--check-address` and `--check-restrict`. You must specify at least one check for the tool to run.

### Passing arguments

You must specify the number of dimensions using `--work-dim`.

You must specify the numbers of global work-items using `--global-work-size`, one time for each dimension.

You can specify the sizes of work-groups using `--local-work-size`, one time for each dimension.

You should pass kernel arguments in the same order as they declared in the kernel function using the repeatable `--arg` argument.

You can pass integer or buffer arguments, as well as skip arguments using `undefined`.
It is strictly recommended to pass all arguments possible.

Integer arguments are passed as numbers followed by an optional postfix that describes their type.
Supported postfixes are `i8`, (`int8_t`), `u8` (`uint8_t`), `i16`, (`int16_t`), `u16` (`uint16_t`),
`i32`, (`int32_t`), `u32` (`uint32_t`), `i64`, (`int64_t`), `u64` (`uint64_t`). The default one is `i64`.

Buffer arguments are passed as `b` followed by their size in bytes. Their contents are not provided.

### Example

`clsa --input src\test.cl --kernel vecadd --work-dim 2 --global-work-size 16 --global-work-size 32 --local-work-size 1 --local-work-size 2 --arg b16 --arg undefined --arg 10u16`

## Building

You'll need to install the following packages and their dependencies:
 - `llvm-12`
 - `clang-12`
 - `libclang-12-dev`
 - `oclicd-opencl-dev`

The whole project can be built as the command line tool, while the contents of the `lib` folder can also be built as a shared library (`so`/`dll`).
To build the command line tool you can use the bundled `CMake` file.

## Using the C++ Library

All you need will be imported by the `lib/frontend/analyzer.h` header.

Create an `clsa::analyzer` instance providing the path to the file containing the code.

Use `analyzer.set_violation_handler` the set the handler which will be invoked each time the analyzer identifies a problem.

Then use `analyzer.analyze` method feeding it the kernel name and its arguments. In arguments, you can use either real CL buffers or create pseudo-buffers using the `clsa::pseudocl_create_buffer` function. Don't forget to release them using `clsa::pseudocl_release_mem_object` when you're done!

## Using the C API

You can also use the analysis library from pure C code using the wrapper provided by the `lib/c_api.h` header.

Invoke `clsa_analyze` feeding the path to the file containing the code, the kernel name and its arguments, as well as a pointer a size_t variable for the analyzer to write the total count of found problems. In arguments, you can use either real CL buffers or create pseudo-buffers using the `clsa_create_buffer` function. Don't forget to release them using `clsa_release_mem_object` when you're done!
The function will a return a pointer to the array of found issues represented as `clsa_violation`s.

## Architecture - A brief description

The analyzer consists of 3 main parts.

### Core

All the fundamental logic is located here.

`ast_visitor` traverses the kernel and holds a `block` context with all the variables, their versions and other various metadata along their Z3 representations.
Visitor unfolds loops and function calls upon the set limit. When the execution flow branches, the visitor processes each branch separately creating a fork of the context for each of them,
merging the forks back afterwards. When the visitor encounters a memory access, it lets the set checkers do their job.

### Checkers

Checkers are relatively small classes that each perform their own checks on a constraint. Currently, there are `address_checker` and `restrict_checker`.
A set of checkers is provided to the Core, which then invokes each checker on each memory access providing them the details about this access,
including the access type (`read`/`write`), its address represented in the form of a Z3 expression, and the current block context.

### Frontend

Frontend is responsible for setting up Clang, transforming all the arguments into the format required by the Core, and starting the analysis process.
