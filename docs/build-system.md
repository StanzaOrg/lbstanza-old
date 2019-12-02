# Using Stanza's Build System

Stanza comes with a built-in build system for helping to build complicated Stanza projects. This document shows you how to use it.

# Simple Project with .stanza Files

Let's assume that your project has the following `.stanza` files:

Inside `mydir/fileA.stanza`:

```
package mypackageA :
  import core
  import collections
  
println("This is package A")
```

Inside `mydir/fileB.stanza`:

```
package mypackageB :
  import core
  import collections
  
println("This is package B")
```

Inside `mydir/fileC.stanza`:

```
package mypackageC :
  import core
  import collections
  import mypackageA
  import mypackageB
  
println("This is package C")
```

## Creating a stanza.proj File

Create the following `mydir/stanza.proj` file:

```
package mypackageA defined-in "fileA.stanza"
package mypackageB defined-in "fileB.stanza"
package mypackageC defined-in "fileC.stanza"
```

These statements tell the Stanza compiler which files to look in to find the definition of each Stanza package.

## Calling the Compiler

In a new shell, navigate to `mydir` and type the following to compile `mypackageC`:

```
stanza mypackageC -o myapp
```

This will tell the Stanza compiler to compile `mypackageC` to an application called `myapp`. Because `mypackageC` imports `mypackageA` and `mypackageB`, the Stanza compiler will pull in the files containing those packages too.

## Other .proj Files

Note that the file called `stanza.proj `is always read by the Stanza compiler to determine where files are located. If you have additional `.proj` files, e.g. `custom.proj`, you can provide them as an additional input to the Stanza compiler like so:

```
stanza custom.proj mypackageC -o myapp
```

## Caching .pkg Files

You can use Stanza's incremental compilation feature, by caching the `.pkg` files created by the Stanza compiler. 

Create a folder called `mydir/pkgs`, and run the following command:

```
stanza mypackageC -o myapp -pkg pkgs
```

This will compile `mypackageC` to the application `myapp` and save any intermediate `.pkg` files to the `pkgs` folder. If you then go to compile `mypackageC` again, Stanza will pull in the cached `.pkg` files automatically if their corresponding source files have not changed.

## Environment Variables

Stanza allows the use of environment variables in the `.proj` file. Suppose that `fileC.stanza` is actually in a directory specified by the `FILE_C_INSTALLATION` environment variable. Then we can accomodate this with the following change to `stanza.proj`:

```
package mypackageA defined-in "fileA.stanza"
package mypackageB defined-in "fileB.stanza"
package mypackageC defined-in "{FILE_C_INSTALLATION}/fileC.stanza"
```

## {.} and {WORKDIR}

There are two special variables that are available to you in the `.proj` file called `.` and `WORKDIR`. 

The `WORKDIR` variable refers to the absolute path to the current working directory, the directory from which the Stanza compiler was ran.

The `.` variable refers to the absolute path to the directory containing the `.proj` file. 

These two special variables will be handy for compiling foreign files.

# Multi-Library Projects

## Project Structure

Suppose that we are working on the application in the previous section of this document, and we now need to include a library project in the directory `/mypath/to/mylib`, which has its own `.stanza` files and dependency structure.

## Include Statement

Assuming that `mylib` has a `.proj` file called `/mypath/to/mylib/stanza.proj`, we can include these dependencies by modifying `mydir/stanza.proj` like so:

```
include "/mypath/to/mylib/stanza.proj"
package mypackageA defined-in "fileA.stanza"
package mypackageB defined-in "fileB.stanza"
package mypackageC defined-in "fileC.stanza"
```

# Foreign File Dependencies

Suppose that our package `mypackageC` requires some functions declared in the foreign files `chelpers.c` and `cpphelpers.o`, and needs to be compiled with the flags `-lmyhelpers`. We can tell Stanza to automatically pull in these dependencies by adding the following line to our `stanza.proj` file:

```
package mypackageC requires :
  ccfiles:
    "chelpers.c"
    "cpphelpers.o"
  ccflags:
    "-lmyhelpers"
```

## Platform-Specific Files and Flags

Suppose that `mypackageC` requires the `-lmyhelpers` flag only when compiling on the `os-x` platform. We can specify this like so:

```
package mypackageC requires :
  ccfiles:
    "chelpers.c"
    "cpphelpers.o"
  ccflags:
    on-platform :
      os-x : "-lmyhelpers"
      else : ()
```

## Running a Foreign Compiler

Suppose that `cpphelpers.o` is created by calling a foreign compiler (e.g. `g++`). We can request Stanza to run a given shell command whenever it compiles an app that depends upon `cpphelpers.o` like so:

```
package mypackageC requires :
  ccfiles:
    "chelpers.c"
    "cpphelpers.o"
  ccflags:
    "-lmyhelpers"
    
compile file "cpphelpers.o" from "cpphelpers.cpp" :
  "g++ {.}/cpphelpers.cpp -c -o {.}/cpphelpers.o"
```

Note the use of the special `.` variable. Recall that it refers to the absolute path of the directory containing the `.proj` file. Our use of `{.}` allows the `g++` call to work no matter which directory it is ran from. 

Note that the `on-platform` construct works for the `compile` construct as well.

If it is a flag that requires calling a foreign compiler, and not a file, then you must use the `compile flag` construct instead. Here is an example:

```
compile flag "-lcurl" :
  "cd mypath/to/curl && make"
```

# Conditional Imports

Suppose that we are working on the following package `animals`, which contains the following definitions:

```
defpackage animals :
  import core
  import collections
  
public defstruct Dog :
  name: String
  weight: Double
  breed: String
```

## Adding Support for JSON

Now suppose that we wish to support converting `Dog` objects to JSON strings. To do this, there is a library in the package `json-exporter` containing a single multi called `to-json`, and we need to add a new method to support `Dog`.

```
defpackage animals :
  import core
  import collections
  import json-exporter
  
public defstruct Dog :
  name: String
  weight: Double
  breed: String
  
defmethod to-json (d:Dog) :
  to-string("{name:%~, weight:%_, breed:%~}" % [name(d), weight(d), breed(d)])
```

However, this change makes the `animals` package dependent upon the `json-exporter` package! This means that any application that requires the `Dog` datastructure will also need to pull in the `json-exporter` package, even if the application doesn't require JSON support at all!

To overcome this, we use the conditional imports feature in the build system.

## Separating the JSON functionality

Let's move the JSON support for animals into a different package:

```
defpackage animals/json :
  import core
  import collections
  import json-exporter
  import animals
  
defmethod to-json (d:Dog) :
  to-string("{name:%~, weight:%_, breed:%~}" % [name(d), weight(d), breed(d)])
```

This will allow any app to optionally include the `animals/json` package if they require JSON support, or omit it if they don't. 

## Automatic Import

Even better, we can ask the Stanza build system to *automatically* pull in the `animals/json` package whenever we need both the `animals` package and the `json-exporter` package together.

The resulting `stanza.proj` file looks like this:

```
package animals defined-in "animals.stanza"
package animals/json defined-in "animals-json.stanza"
package json-exporter defined-in "json-exporter.stanza"
import animals/json when-imported (json-exporter, animals)
```

# Build Targets

Suppose that the following is the final compilation command that you use to build your application:

```
stanza mylib1 myapp -o myapplication -ccfiles "runtime.c" -optimize -pkg "mypkgs"
```

You can store this information in a build target in your `stanza.proj` file using the following syntax:

```
build full-application :
  inputs:
    mylib1
    myapp
  o: "myapplication"
  ccfiles: "runtime.c"
  pkg: "mypkgs"
  optimize
```

Then you can choose to build this target using the following command:

```
stanza build full-application
```

If no target name is provided:

```
stanza build
```

then Stanza will assume that there is a target named `main`. 

One advantage of using build targets is that Stanza will automatically track the files that the compilation process depended upon. If these files haven't changed since you last built the target, then Stanza will print out a message like the following:

```
Build target full-application is already up-to-date.
```

