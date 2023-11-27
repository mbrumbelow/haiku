# Enhancing the IDE experience

It's possible to have full live IDE code completion while working on the Haiku codebase from Linux
in your favorite text editor or IDE.

# Generating a compilation database

## Pre-requirements

* Complete Haiku development environment
  * Haiku checked out locally
  * Buildtools checked out locally
  * Jam installed
* [clangd](https://clangd.llvm.org) or an editor with clangd built-in like [helix](https://helix-editor.com)

## Process

> Make sure to run bear on a "clean" generated directory. We want it to capture all compiled files

* ```mkdir generated.clangd && cd generated.clangd```
* ```../configure --build-cross-tools x86_64 --cross-tools-source ../../buildtools -j4```
* ```jam -qanc -j4 @nightly-anyboot```
* ```ln -sr compile_commands.json ../compile_commands.json```

# Using

Now, when you use an editor which supports clangd, clangd will have knowledge of all of the
relevant include paths and build flags for every source file within the Haiku codebase.  When large
new portions of code are added however, you will need to run though the process above for a complete
database.

> Warning: Please do not commit compile_commands.json to haiku's source tree. It is specific
> to your development environment.
