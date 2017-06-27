# Framework X -- Proof-of-concept of template-based transformations in C++ using Clang and LLVM
This project is a proof-of-concept and as such is very likely to contain inefficiencies or bugs.

# Building the Source Code
This project requires a full installation of Clang and LLVM in order to be built.
Before the first build of the project, users need to run the ``setup.sh`` script. This script will compile the necessary dynamic libraries used in the project.
Xcode users can build the source code using Xcode's internal build system. A number of project parameters may need to be adjusted, such as the header search paths and the linker library search flag in order to find the necessary Clang and LLVM components in your installation. You may also need to adjust the product scheme (``Product >> Scheme >> Edit scheme``) to correct the ``-resource-dir`` parameter (see below).
The source code can also be built manually using CMake. It suffices to run the ``build.sh`` script in the project directory.

## Resource Directory
To correctly parse the source files, and to support the standard library headers, the tool executable must be able to find the system include directory. There are two ways to guide the tool towards this directory.

1. After compiling the transformation tools, the resulting executable can be copied to the directory where the Clang binary executable resides. The tool will then find the resource directory relative to this directory automatically.
2. When running the tool, users may pass the ``-resource-dir=<dir>`` flag via the command line arguments. E.g. on macOS, using version 4.0.0 of Clang/LLVM installed through Homebrew, this would take the form of ``-resource-dir=/usr/local/Cellar/llvm/4.0.0/lib/clang/4.0.0``

## A Note for macOS Users
Mac users must ensure that the Clang executable available in their shell `PATH` is the one supplied with the official releases, as the command line tools shipped with Xcode do not include all development libraries and headers, meaning that the default Clang executable is unable to find them. The easiest way of installing the full release of Clang and LLVM is by using Homebrew and extending the `PATH` variable to point to the install directory used by Homebrew.

# Running the Transformation Tools
After the built transformation executables have been copied to the correct directory, they can be run as normal shell commands. Each of the source files to be transformed must be passed as a command line argument to the tool. To be able to correctly analyze the source files, the Clang tool needs to know how to compile them. The easiest way to do this is by creating a compilation database, as described in this [Clang documentation page](http://clang.llvm.org/docs/HowToSetupToolingForLLVM.html#setup-clang-tooling-using-cmake-and-make) and this [blog post](http://eli.thegreenplace.net/2014/05/21/compilation-databases-for-clang-based-tools), the tool will then automatically recognize the compilation database on the filesystem and utilize it.

If you are unable to create a database, the compilation flags can be passed on the command line as well. To do this, invoke the tool using this syntax: `tool <src ...> -- <compilation_flags>`. If no compilation flags are needed and no compilation database is available, the compilation flags in the above command can be left empty, like this: `tool <src ...> --`. The `--` is still necessary to suppress warnings of unavailable compilation databases.

# Third-Party Libraries
This project uses a number of open-source, third-party libraries, most notably the LLVM project, found at [llvm.org](http://llvm.org/). Other libraries include:

- [JSON for Modern C++](https://github.com/nlohmann/json)
- [Modern C++ JSON schema validator](https://github.com/pboettch/json-schema-validator)
