# Earley Parser #

## Types of Terminals ##

### Keyword Terminal ###
```
GKeyword :
  item: Symbol
```

This terminal matches against an upcoming symbol that matches exactly.

### Primitive Terminals ###
```
GCharToken
GByteToken
GIntToken
GLongToken
GFloatToken
GDoubleToken
GStringToken
GSymbolToken
GTrueToken
GFalseToken
```

These terminals match against upcoming primitives with the appropriate type. 

### List Start Terminal ###
```
GListStart :
  reluctant?: True|False
```

A `GListStart` matches against an upcoming list. If the `GListStart` is non-reluctant then upcoming lists are forced to be expanded. 

If the `GListStart` is reluctant, then it matches against an upcoming list only if the input stream does not match against a non-reluctant terminal.

### List End Terminal ###
```
GListEnd
```

A `GListEnd` matches the end of a list in the input stream.

### List Rest Terminal ###
```
GListRest
```

A `GListRest` matches only if the input stream does not match against a non-reluctant terminal.

### Any Terminal ###
```
GAny :
  type: Atomic|Reluctant|Standard
```

A standard `GAny` matches against any form.

A reluctant `GAny` matches against any form if a reluctant match is allowed.

An atomic `GAny` matches against any form, but if matched, then the input stream is not allowed to expand and parse within the next upcoming s-expression (in the case where the upcoming s-expression is a list). 

## Reluctant Matching ##

Terminals are classified into either reluctant or non-reluctant terminals, where the standard terminals are all classified as non-reluctant terminals. 

For a given position in the input stream, reluctant terminals are only allowed to match if the input stream matches against some expected non-reluctant terminal.

## Explanation of Parsing Tables ##

Here is an example of the parsing tables:
```
Set 0
  (rule 0) [Start = • X X X AS, S0]
  (rule 5) [X = • x, S0]
Set 1
  (rule 5) [X = x •, S0]
  (rule 0) [Start = X • X X AS, S0]
  (rule 5) [X = • x, S1]
Set 2
  (rule 5) [X = x •, S1]
  (rule 0) [Start = X X • X AS, S0]
  (rule 5) [X = • x, S2]
Set 3
  (rule 5) [X = x •, S2]
  (rule 0) [Start = X X X • AS, S0] (complete as (rule 0) [Start = X X X • AS, S0])
  (rule 1) [AS = • A X BS, S3]
  (rule 6) [A = • a, S3]
Set 4
  (rule 6) [A = a •, S3]
  (rule 1) [AS = A • X BS, S3]
  (rule 5) [X = • x, S4]
Set 5
  (rule 5) [X = x •, S4]
  (rule 1) [AS = A X • BS, S3] (complete as (rule 0) [Start = X X X • AS, S0])
  (rule 2) [BS = • B X CS, S5]
  (rule 7) [B = • b, S5]
Set 6
  (rule 7) [B = b •, S5]
  (rule 2) [BS = B • X CS, S5]
  (rule 5) [X = • x, S6]
Set 7
  (rule 5) [X = x •, S6]
  (rule 2) [BS = B X • CS, S5] (complete as (rule 0) [Start = X X X • AS, S0])
  (rule 4) [CS = • C X AS, S7]
  (rule 8) [C = • c, S7]
...
```

There are three main types of items:

### Standard Item ###
```
Set 15
  (rule 7) [A = B C • D, S10]
```
This says the input from position 10 to position 15 can be successfully parsed as the first two tokens of rule 7 (B C), for production A.

### Right-Recursive Item ###
```
Set 7
  (rule 2) [BS = B X • CS, S5] (complete as (rule 0) [Start = X X X • AS, S0])
```
This says the input from position 5 to position 7 can be successfully parsed as the first two tokens of rule 2 (B X) for production BS. Furthermore, when the last production for this rule (CS) is parsed, and this rule is completed, it will immediately result in the completion of a chain of other rules, ending ultimately with the completion of rule 0 started at position 0.

If the completion root is equal to the item then this is the starting item of a right-recursive chain.

### Completed Right-Recursive Item ###
```
Set 20
  (rule 0) [Start = X X X AS •, S0] (complete as (rule 2) [BS = B X CS •, S17])
```
This says that, at position 20, rule 0 has been completed as the result of the completion of a chain of rules originating from the completion of rule 2 started at position 17.

## Backward Parse ##

After completing the forward parse, we need to run the Earley parser again upon the reversed input. 

The terminals that need special attention are:
- reluctant list start
- list rest
- reluctant any

The reverse parse is straightforward, except that we replace these special terminals with a marker that records what position they matched upon in the forward parse. 

## Earley Search ##

### State ###
```
setlist: ESetList
inputlist: Vector<SExpToken>
infolist: Vector<FileInfo|False>
missing: Vector<MissingInput>
```

### Process Set ###

This function iterates through every Earley item in `current-set`, matches them against the given input, advances them, and puts them into `next-set`. If `include-all-rules?` is true, then prediction items cause all possible rules to be considered. If `include-all-rules?` is false, then prediction items only cause rules that can match against the next input to be considered. 

```
Function `process-set`
Input:
  set-index: Int
  current-set: ESet
  next-set: ESet
  next-input: SExpToken
  include-all-rules?: True|False
Uses:
  prediction-set: ProductionSet
  completion-set: CompletionSet
```

For each item in the current set, we look at its upcoming token. There are three cases:
Case: A terminal is upcoming
Case: A production is upcoming
Case: End of rule

#### Case: A terminal is upcoming #### 
Test whether the given terminal matches against the given input. If it does, then advance the item, and add it to the next set.

#### Case: A production is upcoming #### 
First check whether the production is nullable. If so, then advance the item, and use "ensure-added" to add it to the current set.
Because a production is upcoming, we have to add all of this production's rules to the current set if we have not already done so. Tracked using `prediction-set`.

####  Case: End of rule #### 
Check whether it is a trivial completion. A trivial completion is a rule that matched against a list of zero tokens.
If it is not a trivial completion, then we need to complete the production, if it has not already been completed. Look at all the items in the parent set that had this production as the upcoming item and advance past it. The completed item is added to the current set using "ensure-added".

Completing the item: If the item has a completion root (and it is not the first in the chain), then we use the advanced root as the completed item instead of the original item. The original item is stored in the completion root to indicate the originating completion. The objective is to create a "completed right-recursive item".

#### Utilities ####
ensure-added: This adds the given item to the current set if it has not already been added. Tracked using `completion-set`. 
