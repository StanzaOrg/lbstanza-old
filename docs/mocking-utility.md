# Using Stanza's Mocking Utility #

Stanza comes with a built-in mocking utility to make it easier to create mock objects for testing system components in isolation. 

# Example #

Look at `examples/simplemock.stanza` in the Stanza distribution.

Compile and run it like this:

```
stanza compile-test examples/simplemock.stanza -o simplemock
./simplemock
```

# MockScript Utility #

The `MockScript` utility, defined in `stz/mocker`, is a simple coroutine-based utility that allows you to write a simple "script" that anticipates calls to it, and then replies with appropriate values. 

## Calling a MockScript ##
@[Mocking Call Script Definition]

To start, let's delay talking about the details of how to write a mocking script, and instead presume that we are given an already-created `MockScript` object.

From the perspective of the caller, a `MockScript` object supports one primary function: the ability to call it.
```
defn call (script:MockScript, message:Tuple) -> ?
```

You call a script with a *message*, which is just a tuple containing a tag and some arguments, and the script will reply with some value.

Here is an example of calling a script with a message requesting to add two numbers.
```
val result = call(script, [`add-nums, 10, 23])
```

The first element in the tuple must be a symbol, and is what we call the "message tag". 

The `call` function returns the value calculated by the mocking script in reply to the given message, whatever it turns out to be.

## Creating a MockScript ##
@[Mocking Constructor Definition]

The constructor function for a `MockScript` has the following signature
```
defn MockScript (body:MockCaller -> ?) -> MockScript
```

but it is usually called using the `within` macro shorthand:
```
val script =
  within caller = MockScript() :
    ... body ...
```

Within the body of the script, the `MockCaller` object is a representation of the environment that is calling the script. 

### Expect and Reply ###

Within the body of a `MockScript`, there are two important functions to use.

The `expect` function waits until the next time the script is called, and returns the message it is called with.
```
defn expect (caller:MockCaller, message:Tuple) -> Tuple
```

Here is an example of a script expecting to be called with our previous message for adding two numbers.
```
expect(caller, [`add-nums, 10, 23])
```

Note that the script will throw an exception if the caller does not call the script with the expected message.

Once the script has received the message it expects, it can then use the `reply` function to return a result to the caller. 
```
defn reply (caller:MockCaller, value) -> False
```

Thus, now putting the two halves together, the part of the script for mocking the call to add two numbers looks like this:
```
expect(caller, [`add-nums, 10, 23])
reply(caller, 33)
```

## Other Forms of `Expect` ##
@[Mocking Expect Definition]

We previously showed you only one form of the `expect` function. Here is the full list:

### Expect Any Message ###
Retrieve the message that the script is next called with. Has no expectation on the message received. 
```
defn expect (caller:MockCaller) -> Tuple
```

Examples:
```
val msg = expect(caller)
```

### Expect Message With Tag ###
Retrieve the message that the script is next called with. Expects for the message to have a given tag, and throws an exception otherwise.
```
defn expect (caller:MockCaller, message-type:Symbol) -> Tuple
```

Example: Expect a message tagged with `add-nums`, but with any arguments.
```
val msg = expect(caller, `add-nums)
val arg0 = msg[1] as Int
val arg1 = msg[2] as Int
```

### Expect Exact Message ###
Retrieve the message that the script is next called with. Retrieved message must match given message exactly (as determined by `equal?`). 
```
defn expect (caller:MockCaller, full-message:Tuple) -> Tuple
```

Example: Expect a `add-nums` message with arguments 10 and 23.
```
expect(caller, [`add-nums, 10, 23])
```

# Example Walkthrough #

The `simplemock.stanza` example shows how we can use the `MockScript` utility to write a test for an algorithm called `recursive-list-files`. The algorithm recursively traverses an installation directory and finds the paths to all files contained in the directory. 

The algorithm is carefully designed to allow for easy testing by accepting an `InstallDir` object that supports all of the necessary system interactions.

The `InstallDir` object needs to support only two operations:

The `files` function which returns a tuple containing all of the immediate files inside a directory.
```
defmulti files (dir:InstallDir, file:String) -> Tuple<String>
```

The `directory?` function which returns `true` if the given file is a directory, or `false` otherwise.
```
defmulti directory? (dir:InstallDir, file:String) -> True|False
```

## A Mock `InstallDir` Object ##
@[mocking mock InstallDir implementation]

The example uses the function
```
defn MockInstallDir () -> InstallDir
```

to create a mock `InstallDir` object used solely for testing. It is meant to represent the following directory:

```
myfile.txt
myprog
mydir/
  a.txt
  b.png
```

The test is free to create and return a new `InstallDir` object however it wishes. But as we will see, by making use of the `MockScript` utility, we can easily write an `InstallDir` object that implements exactly enough for a particular test.

### Forwarding Methods to a Script ###

The strategy used by `MockInstallDir` is to create a `MockScript` and then create a `InstallDir` object that simply forwards its methods to the script.

```
defn MockInstallDir () :
  val script =
    within caller = MockScript() :
      ... body of script ...
  new InstallDir :
    defmethod files (this, file:String) :
      call(script, [`files, file])
    defmethod directory? (this, file:String) :
      call(script, [`directory?, file])
```

Calls to `files` will use messages tagged with the symbol `files`, and calls to `directory?` will use messages tagged with the symbol `directory?`. 

Note that this example forwards *all* methods to the script, but this is not necessary. Sometimes, it is easier to just implement a simple version of the method directly. 

### The Script Body ###

The script implements the bare skeleton needed to test the `recursive-list-files` algorithm.

The script first expects for the algorithm to list all files in the "." directory.
```
expect(caller, [`files, "."])
reply(caller, ["myfile.txt", "myprog", "mydir"])
```

Then the script expects the algorithm to test each of them to determine whether they are directories.
```
expect(caller, [`directory?, "./myfile.txt"])
reply(caller, false)

expect(caller, [`directory?, "./myprog"])
reply(caller, false)

expect(caller, [`directory?, "./mydir"])
reply(caller, true)
```

Since `mydir` is a directory, the script then expects the algorithm to list its files.
```
expect(caller, [`files, "./mydir"])
reply(caller, ["a.txt", "b.png"])
```

And finally, the script expects the algorithm to test each of these files to determine whether they are directories.
```
expect(caller, [`directory?, "./mydir/a.txt"])
reply(caller, false)

expect(caller, [`directory?, "./mydir/b.png"])
reply(caller, false)
```

### The Test ###
@[mocking recursive-list-files test]

The final test is simple.

```
deftest test-recursive-list-files :
  val files = recursive-list-files(MockInstallDir())
  #ASSERT(files == ["./myfile.txt" "./myprog" "./mydir/a.txt" "./mydir/b.png"])
```  

It simply calls `recursive-list-files` with our mocked `InstallDir` object, and tests whether the result is what we expect. 
