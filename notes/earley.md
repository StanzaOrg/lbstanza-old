# Earley Parser #

## SExpTokens ##

### Standard SExp Forms ###
```
SExpForm :
  form
  list:List
```

This represents a single s-expression form. For implementing matching against list rest, we also provide all subsequent forms in the `list` field. 

### End of List ###
```
SExpListEnd
```

This represents the end of a list in the input. 

### Wildcards ###
```
SExpWildcard
```

### End of Input ###
```
EndOfInput
```

This represents the end of the input.

This is a special token that can be inserted into the input stream in order to force a match against an upcoming terminal.

## SExpStream ##

A `SExpStream` represents the input list of tokens. It is constructed from a single s-expression list.

### Retrieving the Next Token ###

The `peek` and `info` functions return the upcoming token and its file information. 

```
defmulti peek (s:SExpStream) -> SExpToken
```

```
defmulti info (s:SExpStream) -> FileInfo|False
```

Remember that all `SExpStream` objects end with a `SExpListEnd` followed by a `EndOfInput` token.

### Advancing the Stream ###

The `advance` function steps the stream past the next upcoming token. If `expand-list?` is passed true, then the stream recursively enters the next upcoming list (if it is a list). 

```
defmulti advance (s:SExpStream, expand-list?:True|False) -> False
```

The `advance-rest` function steps past all tokens at the current list-level, ending just before `SExpListEnd`. Thus `peek` should return `SExpListEnd` after calling `advance-rest`. 

```
defmulti advance-rest (s:SExpStream) -> False
```

It is an error to advance the stream when the upcoming token is `EndOfInput`. 

### Error Recovery Using Wildcards ###

Wildcards match against any terminal except for ListStart, so the stream allows us to insert a wildcard token to the beginning of the stream.

```
defmulti insert-wildcard (s:SExpStream) -> False
```

The stream allows us to also insert an empty list to the beginning of the stream, to recover from if the parser is stuck awaiting a ListStart.

```
defmulti insert-list (s:SExpStream) -> False
```

It is an error to insert any tokens into the stream if the upcoming token in `EndOfInput`. 

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

## Matching Algorithm ##

The general matching algorithm is handled by:
```
Function matches-input?
Input:
  terminal:GTerminal
  input:SExpToken
Output:
  result:True|False
```

It is assumed that the input is never `EndOfInput`. 

### Wildcards ###
The wildcard token matches against all terminals except for `GListStart` and `GListEnd`.

### SExpForm ###
The SExpForm token matches against all

- keyword
- primitive
- list start
- any
- list rest

terminals, if their contents are appropriate.

### SExpListEnd ###
The SExpListEnd token matches only against `GListEnd`. 

## Reluctant Matching ##

The reluctant matching system is used to prevent input s-expressions from being expanded when unnecessary, and to allow list rest matching to work.

### Special Terminals ###

The reluctance matching system concerns the following terminals:
- Standard GListStart
- Reluctant GListStart
- Atomic GAny
- Standard GAny
- Reluctant GAny
- GListRest

### Reluctant Terminals ###

The following terminals are classified as reluctant, all others are non-reluctant terminals:
- Reluctant GListStart
- Reluctant GAny
- GListRest

We say that a "non-reluctant terminal has been scanned", if a terminal has been scanned, and that terminal is not classified as reluctant.

### Algorithm ###

The reluctant matching system is implemented via a pruning procedure to eliminate reluctant matches from the Earley set. 

This function removes items from the next set to satisfy the reluctant matching rules. It returns true if the matches require the input stream to expand the upcoming list.

```
Function `prune-reluctant-matches`
Input:
  eset:ESet
Output:
  expand?:True|False
```

There are three initial conditions to calculate:
- scanned-non-reluctant: true if a non-reluctant terminal has been scanned.
- scanned-standard-list-start: true if a Standard GListStart has been scanned.
- scanned-atomic-any: true if an Atomic GAny has been scanned.

From these initial conditions, we can compute whether or not we should expand the upcoming list.
- expand: true if scanned-standard-list-start and not scanned-atomic-any

Now, iterate and test each item in the Earley set. For each
- Standard GListStart: Keep if expand.
- Reluctant GListStart: Keep if expand.
- Standard GAny: Keep if not expand.
- Reluctant GAny: Keep if not expand and scanned-non-reluctant.
- GListRest: Keep if not scanned-non-reluctant.
- otherwise: Keep.

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

## SavedMatches ##

The `SavedMatches` datastructure is used to remember, for special terminals, which positions in the input stream they last matched.

The `save-match?` function returns `true` if the given terminal is a "special" terminal, and we need to remember the positions that they matched against. @[SavedMatches save-match?]

If a special terminal matches at a given position, we use the `save` function to record that the terminal matched at the given position. @[SavedMatches save]

To query the SavedMatches datastructure we use the `matched?` function to ask whether a given terminal previously matched at a given position. @[SavedMatches matched?]

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

### Computing the Completion Root ###

```
Function `compute-completion-root`
Input:
  set-index: Int
  current-set: ESet
Uses:
   production-count: ProductionTable<Int>
```

Compute the completion-root for each deterministic-reduction in the set.

A deterministic reduction looks like this:
```
(rule 0) [Start = X X X • AS, S0]
```
It has one remaining production to parse, and this item is the only item with that upcoming production. 

The completion of a deterministic reduction is calculated like so:
First search for a completion-root in the parent. Let `pitem` be the first item for production `Start` started at position 0. If `pitem` has a completion-root, then this is the completion root for the item.
If no completion-root is found in the parent, then the item is its own completion-root. 

### Main Algorithm ###

Each iteration manages
```
current-set:ESet
next-set:ESet
```

We first process the `current-set` and add advanced items to `next-set`. Then we prune the reluctant matches from `next-set`.

At this point, there are a few scenarios:

```
Case: Next set is empty.
  Case A: The current set contains the completed start production.
  Case B: The current set does not contain the completed start production.
Case C: Next set is not empty.
```

#### Case A: Finished Parse ####
This case occurs when we have finished parsing.

#### Case B: Error Recovery ####
This case occurs when the parser is stuck and is awaiting a terminal that does not match the input stream. 

First, we want to save the context that caused this error. So compute the first list of Earley items (without using predictive look-ahead), and save this information.

Here is the full list of terminals awaiting, and the action to take to force the parser to continue:
- Keyword: Insert wildcard.
- Primitive terminals: Insert wildcard.
- List start: Insert list.
- List end: Drop input.
- Any: Insert wildcard.

In terms of priority, we want to take actions with the following priority:
1. Insert wildcard
2. Insert list
3. Drop input

#### Case C: Standard Iteration ####
This case occurs when a normal iteration is completed.
We need to advance the input stream, and then go to the next iteration.
To advance the input stream we either advance to the end of the list if the rest terminal was scanned, or by a single form otherwise. 

## Creation of Parse Forest ##

The input to the parse forest algorithm is the set list from the backward parse. 

The most important interface to the parse forest is the `get` function:
```
defmulti get (f:ParseForest, r:ParsedRange, index:Int, starting-position:Int) -> Seqable<ParsedRange>
```

where `ParsedRange` is defined as followed:
```
defstruct ParsedRange :
  rule: Int
  start: Int
  end: Int
```

The input `ParsedRange` represents a range of input tokens, between `start` (inclusive) and `end` (exclusive), that can be parsed according to the given rule. As an example, the root range for a successful parse is `ParsedRange(start-rule, 0, total-num-tokens)`, where `start-rule` is the identifier of the starting rule, and `total-num-tokens` is the total number of tokens in the input. 

The `ParseForest` allows us to query the individual subtrees that make up a `ParsedRange`. Suppose the input parsed range is
```
(rule 4) [F = A c d A B] from 10 to 35
```
. Then the question we are interested in is:

"What can the subproductions `A` (first occurrence), `A` (second occurrence), and `B` be parsed as?"
 
To answer this question for the first occurrence of `A`, we ask the parse forest:

"Given `ParsedRange(4, 10, 35)`, what are the parsed ranges for token 0, starting from 10?"

And suppose that the parse forest returns two ranges:
```
ParsedRange(22, 10, 15)
ParsedRange(23, 10, 18)
```
This means that tokens 10 to 15 can be parsed as the first occurrence of `A` using rule 22, and that tokens 10 to 18 can also be parsed as the first occurrence of `A` using rule 23. 

Now suppose we choose, using our specificity relation, that we want the parse tree containing subtree `ParsedRange(23, 10, 18)`. From rule 4, the following tokens after `A` are the terminals `c` and `d`. Therefore, the next question to ask the parse forest is:

"Given `ParsedRange(4, 10, 35)`, what are the parsed ranges for token 3, starting from 20?"

### Backwards Parse ###
At the end of a backwards parse, the following item
```
Set 25
  (rule 2) [BS = a • AS, S1]
```
means:

"The tokens after `•` in rule 2 can successfully be parsed from set 25 (counted from the back) and set 1 (counted from the back)."

### Key Algorithm ###

Consider answering this question:

"Given `ParsedRange(4, 10, 35)`, what are the parsed ranges for token 0, starting from 10?"

Now there may be many ranges for the production `A` starting at position 10:
```
ParsedRange(22, 10, 15)
ParsedRange(23, 10, 18)
ParsedRange(24, 10, 21)
ParsedRange(28, 10, 26)
```

But not all of these parses will be compatible with the attempt to parse tokens 10 to 35 using rule 4. In order to be compatible we need the following information:
```
(rule 4) [F = A • c d A B] can be parsed from 18 to 35
(rule 4) [F = A • c d A B] can be parsed from 15 to 35
```
which says that the tokens after `A` in rule 4 can be successfully parsed using input 17 to 35, or input 15 to 35. 

Therefore, only two of the parsed ranges for `A` is compatible:
```
ParsedRange(22, 10, 15)
ParsedRange(23, 10, 18)
```

### Completion Chains ###

Completion items such as below
```
Set 20
  (rule 0) [Start = • X X X AS, S0] (complete as (rule 2) [BS = • B X CS, S17])
```
, can be thought of as implicitly representing an entire stack of items once expanded. The above may represent something akin to the following
```
Set 20
  (rule 2) [BS = • B X CS, S17]
  (rule 4) [CS = • C X BS, S15]
  (rule 2) [BS = • B X CS, S13])
  ...
  (rule 0) [Start = • X X X AS, S0]
```

Thus the general algorithm, when answering the question:

"What are all the possible parses for production `A` starting from set 13?"

We need to first expand all of the items in the set corresponding to set 13. 

### Algorithmic Strategy ###

When requesting all of the valid parses of production `A` starting from position `35`, we first expand all of the completion chains in the set corresponding to position 35. Because we select a particular parse tree at the end, this strategy allows us to avoid a complete expansion of most sets.

## Computing Descriptive Errors ##

Each time there is an unexpected input, we save the context of the parse at that moment in a `MissingInput` datastructure.

@[MissingInput Definition]
```
defstruct MissingInput :
  input
  set-index: Int
  info: FileInfo|False
  items: Tuple<EItem>
```

The goal is to compute a good error message from the list of partially completed EItems. The general strategy is to recognize that many items in the list are explained in as simpler way by other items, and thus do not contribute to a good error message. The following pruning rules apply:

### Remove Items with Matched Wildcards ###

For any items that have matched against wildcards, we assume that these have a high probability of being spurious parse directions. So all items with matched wildcards are removed. 

### Assume that Productions are Complete ###

If possible, we choose to interpret a production as being complete. Thus, if we see the following:
```
(rule 4) [CS =  C X BS •, S15]
```
we assume that the production `CS`, starting from position 15, is complete at this input. We then prune all pending parses of `CS` that started after position 15. 

### Remove Predicted Items ###

Any item that has no tokens yet parsed, such as:
```
(rule 4) [CS = • C X BS, S15]
```
is a "predicted" item, and is therefore explained as the pending production for some other rule, such as:
```
(rule 8) [DS = A x B • CS X, S12]
```
In these cases, it is better to explain the error as failing during a parse of `DS`, expecting a `CS`. 

Thus we prune all predicted items from parses.
