# Using Stanza's Built-in Test Framework

Stanza comes with a built-in testing framework for quickly defining tests. This document shows you how to use it.

# Example

Look at `example/simpletests.stanza` in the Stanza distribution.

Compile and run it like this:

```
stanza compile-test example/simpletests.stanza -o simpletests
./simpletests
```

# Defining Tests

## Importing the testing syntax
```
#use-added-syntax(tests)
```

## How to define a simple test
```
deftest mytest :
  val x = 1 + 3
  #ASSERT(x == 4)
```

A pass fails is:

1. An `#ASSERT` statement fails, or
2. an uncaught exception is thrown, or
3. a fatal error occurs.

## How to add a tag to a test

```
deftest(flaky) mytest :
  val x = 1 + 3
  #ASSERT(x == 4)
```

## How to define a test with a computed name

```
val a = 2
val test-name = to-string("test: a = %_" % [a])
deftest (test-name) :
  #ASSERT(a == 2)
```

## How to include code that is only used for testing

```
#if-defined(TESTING) :
  defn helper-function (x:Int) :
    x + 1
 
deftest mytest :
  val result = 1 + 1
  #ASSERT(result == helper-function(1))
```

`deftest` is never included unless the source file is compiled as a test.

# Running Tests

## Compiling a test executable

```
stanza compile-test mytests.stanza -o mytests
```

## Running the test executable

```
./mytests
```

## Running only the test with the given name

```
./mytests mytest1 mytest2
```

## Running only the tests tagged with a given tag

```
./mytests -tagged short
```

## Running only the tests not tagged with a given tag

```
./mytests -not-tagged flaky
```

