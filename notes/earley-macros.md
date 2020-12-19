# Earley Macros #

## What is Constant and Cached? ##

At run-time, we need to execute the following statement quickly:

```
defn test-parse (msg) :
  match-syntax[mypackage + current-overlays](form) :
    (?x:#exp + ?y:#exp) :
      println(msg)
      println("Add %_ to %_" % [x, y])
    (?ts:#type ...) :
      println(msg)
      println("Types = %," % [ts])
    (_ ...) :
      println(msg)
      println("Other")  
```

The `mypackage` syntax package is constant. The current presiding overlays are not constant. The actions (represented using closures) are not constant, as they potentially close over different values on each invocation. The patterns are constant. 

Let us separate the cached syntax package from the patterns. 

## Preanalyzed Syntax Packages ##

An analyzed syntax package is the result of combining multiple syntax packages into a single syntax package, and precomputing as much as possible. 

To start, we will compute the set of `GRules`, not including the starting productions. 

The analysis algorithm interface is:
```
defn analyze (base:SyntaxPackage,
              overlays:Tuple<SyntaxPackage>,
              tables:SyntaxPackageTables) -> AnalyzedSyntaxPackage|EarleySyntaxErrors
```
