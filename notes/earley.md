# Earley Parser #

# Search Procedure #

Implemented in `stz/earley-search`.

## Search Result ##
@[earley search result]

The result of an Earley search is represented using SearchResult, of which there are two subtypes.

If the search succeeds, then the algorithm returns a SearchSuccess object.
```
defstruct SearchSuccess <: SearchResult :
  items: ECompletionList
  terminal-set: TerminalSet
  inputlist: Vector<SExpToken>
  infolist: Vector<FileInfo|False>
```
- items stores the completed EItem objects.
- terminal-set stores the terminals that match at each position in the input stream.
- inputlist stores the final list of tokens in the input stream.
- infolist stores the computed file information in the input stream.

If the search fails, then the algorithm returns a SearchFailure object.
```
defstruct SearchFailure <: SearchResult :
  missing: Tuple<MissingInput>
```
- missing stores the context for each problem in the input stream.

## TerminalSet ##
@[earley terminal set definition]

A TerminalSet represents the set of all terminals matched at each location in the input stream.

This is the most important function. Returns true if the terminal 'terminal' was matched at index 'index' in the input stream.
```
defmulti get (s:TerminalSet, index:Int, terminal:GTerminal) -> True|False
```

This private function adds a new match to the set. It indicates that the terminal 'terminal' successfully matched at index 'index' in the input stream.
```
defmulti add (s:TerminalSet, index:Int, terminal:GTerminal) -> False
```

For most terminals, it is trivial to recalculate whether or not it matches the input stream. But some terminals involve executing predicates, and we do not want to execute them repeatedly. This function returns true if the terminal 't' should be saved and not recalculated. The TerminalSet only saves the matches for which this function returns true.
```
defn save-terminal-match? (t:GTerminal) -> True|False :
```

## Reluctance Matching System ##

The reluctant matching system is used to prevent input s-expressions from being expanded when unnecessary, and to allow Rest matching to work.

The reluctance matching system concerns the following terminals:
- Standard GListStart
- Reluctant GListStart
- Atomic GAny
- Standard GAny
- Reluctant GAny
- GListRest

### Classification of Terminals ###

@[earley reluctant terminal definition]
Returns true if the given terminal is a reluctant terminal.
```
defn reluctant-terminal? (t:GTerminal) -> True|False
```

The following terminals are classified as reluctant.
- Reluctant GListStart
- Reluctant GAny
- GListRest

@[earley non-reluctant terminal definition]
Returns true if the given token (or false) is a non-reluctant terminal.
```
defn non-reluctant-terminal? (t:GToken|False) -> True|False :
```
A token is a non-reluctant terminal if it is a terminal and it is not a reluctant terminal.

### Algorithm ###
@[earley reluctant matching algorithm]

The reluctant matching system is implemented via a pruning procedure to eliminate reluctant matches from the Earley set. In general, a reluctant terminal matches only if it is forced to match (by some non-reluctant terminal).

This function removes reluctant items from the eset 'eset'. It returns true if the upcoming item should be expanded if it is a list.
```
defn prune-conditional-matches (eset:ESet) -> True|False
```

There are three conditions to calculate:
- scanned-non-reluctant-list-start: true if a non-reluctant list start has been scanned.
- scanned-atomic-any: true if an atomic any has been scanned.
- scanned-non-reluctant: true if a non-reluctant terminal has been scanned.

First we compute whether the upcoming item should be expanded. The upcoming item should be expanded if:
1. A non-reluctant list start is forcing the list to be expanded, and
2. No atomic any is forcing the list to be regarded atomically.

From the expansion calculation, we can remove items according to their type:
List Starts:
  List starts should be removed if the upcoming item should not be expanded.
Any:
  Any is only allowed to match against an atomic form, so it should be
  removed if the upcoming item should be expanded.
  Additionally, if the Any is reluctant, then it should be kept only
  if a non-reluctant terminal has been scanned.
List Rest:
  A list rest should removed if any non-reluctant terminal has been scanned.

## ESet ##
Used to represent the items in the current set and the next set. Has special features for:
- Testing whether the final parse is finished
- Implementing the reluctant matching system
- Implementing the error recovery system

### Basic Functions ###
@[ESet Basic Functions]

Adds the item 'item' to the set 's'.
```
defmulti add (s:ESet, item:EItem) -> False
```

Removes all items from the set 's' after the initial 'length' number of items in the set. It is assumed that the current length is greater or equal to 'length'. The 'length' parameter is used during error recovery to reset the state of the next set to how it was immediately after scanning.
```
defmulti clear (s:ESet, length:Int) -> False
```

Returns the number of items in the set 's'.
```
defmulti length (s:ESet) -> Int
```

During processing of the current Earley set, new items will be added to the set while iterating through the set. The 'do' method is written specifically to handle adding additional items during iteration.
```
defmethod do (f:EItem -> ?, eset:ESet) :
```

### Functions for Testing for Completion ###
@[ESet Completion Test Functions]

Returns true if the starting production (production 0) has been completed. Used to test whether the parse has completed.
```
defmulti start-completed? (s:ESet) -> True|False
```

### Functions for Reluctant Matching System ###
@[ESet Reluctant Matching Functions]

Returns true if the set contains a scanned atomic any terminal.
```
defmulti scanned-atomic-any? (s:ESet) -> True|False
```

Returns true if the set contains a scanned non-reluctant terminal.
```
defmulti scanned-non-reluctant? (s:ESet) -> True|False
```

Returns true if the set contains a scanned non-reluctant list start terminal.
```
defmulti scanned-non-reluctant-list-start? (s:ESet) -> True|False
```

Returns true if the set contains a scanned rest terminal.
```
defmulti scanned-rest? (s:ESet) -> True|False
```

Remove all items in the set satisfying the predicate 'f'.
```
defmulti remove! (f:EItem -> True|False, s:ESet) -> False
```

### Functions for Error Recovery System ###
@[ESet Error Recovery System]

Returns true if the set contains any items expecting a terminal that can match against a wildcard token.
```
defmulti wildcard-expected? (s:ESet) -> True|False
```

Returns true if the set contains any items expecting a list token.
```
defmulti list-expected? (s:ESet) -> True|False
```

Returns true if the set contains any items expecting a list end token.
```
defmulti list-end-expected? (s:ESet) -> True|False
```

## Main Search Algorithm ##
@[earley main search algorithm]

The main algorithm is implemented by the `process-all-sets` algorithm, which uses two main helper functions: `process-set` and `commit-set`. Helper `process-set` completes the current Earley set and adds scanned items to the next Earley set. Helper `commit-set` commits the current set to the Earley set list.

### Explanation of Parsing Tables ###
The main parsing table is the Earley set list.

Here is an example of the parsing tables:
```
Set 0
  (rule 100) [Start = • ( X X X AS ), S0]
Set 1
  (rule 100) [Start = ( • X X X AS ), S0]
  (rule 105) [X = • x, S1]
Set 2
  (rule 105) [X = x •, S1]
  (rule 100) [Start = ( X • X X AS ), S0]
  (rule 105) [X = • x, S2]
Set 3
  (rule 105) [X = x •, S2]
  (rule 100) [Start = ( X X • X AS ), S0]
  (rule 105) [X = • x, S3]
Set 4
  (rule 105) [X = x •, S3]
  (rule 100) [Start = ( X X X • AS ), S0]
  (rule 101) [AS = • A X BS, S4]
  (rule 106) [A = • a, S4]
Set 5
  (rule 106) [A = a •, S4]
  (rule 101) [AS = A • X BS, S4]
  (rule 105) [X = • x, S5]
Set 6
  (rule 105) [X = x •, S5]
  (rule 101) [AS = A X • BS, S4] (completion root)
  (rule 102) [BS = • B X CS, S6]
  (rule 107) [B = • b, S6]
...
Set 19
  (rule 107) [B = b •, S18]
  (rule 102) [BS = B • X CS, S18]
  (rule 105) [X = • x, S19]
Set 20
  (rule 105) [X = x •, S19]
  (rule 102) [BS = B X • CS, S18] (completion root = (rule 101) [AS = A X • BS, S4],
                                   link = (rule 101) [AS = A X • BS, S16])
  (rule 103) [CS = • X, S20] (completion root = (rule 101) [AS = A X • BS, S4],
                              link = (rule 102) [BS = B X • CS, S18])
  (rule 105) [X = • x, S20]
Set 21
  (rule 105) [X = x •, S20]
  (rule 101) [AS = A X BS •, S4] (completion starts = (rule 103) [CS = • X, S20])
  (rule 100) [Start = ( X X X AS • ), S0]
```

There are four main types of items:

### Standard Item ###
```
Set 15
  (rule 7) [A = B C • D, S10]
```
This says the input from position 10 to position 15 can be successfully parsed as the first two tokens of rule 7 (B C), for production A.

### Right-Recursive Roots ###
```
Set 6
  (rule 101) [AS = A X • BS, S4] (completion root)
```
This says that the item is a deterministic reduction. I.e. it is the only item in the set with upcoming production BS. It is also the root of the deterministic chain in that the set at position 4 contains multiple items with upcoming production AS.

### Right-Recursive Links ###
```
Set 8
  (rule 102) [BS = B X • CS, S6] (completion root = (rule 101) [AS = A X • BS, S4],
                                  link = (rule 101) [AS = A X • BS, S4])
```
This says that the item is a deterministic reduction. It is the only item in the set with upcoming production CS. If the item is completed, it will immediately lead to the completion of rule 101 at position 4. And when that rule is completed, it will immediately result in the completion of other items, ending ultimately with the completion of rule 101 started at position 4.

### Right-Recursive Completions ###
```
Set 21
  (rule 101) [AS = A X BS •, S4] (completion starts = (rule 103) [CS = • X, S20])
```
This says that, at position 21, rule 101 has been completed as the result of the completion of a chain of rules originating from the completion of rule 103 started at position 20.

### State of Algorithm ###
@[earley search algorithm state]

Holds the input tokens.
```
input-stream:SExpStream
```

Holds the Earley sets. Each Earley set contains only the items necessary to continue the algorithm, the items that have upcoming productions.
```
setlist:ESetList
```

Holds the context of all errors that occurred during parsing.
```
missing:Vector<MissingInput>
```

Holds the completed Earley items.
```
completion-list:ECompletionList
```

Holds the terminals scanned at each position.
```
terminal-set:TerminalSet
```

### Utility: Creating Starting EItems ###

This function creates a new EItem (with num-parsed set to 0) for the given rule starting at the given location.
```
defn make-eitem (rule:GTokenRule, start:Int) -> EItem
```

### Utility: Memoized Advancing of an EItem ###

This function advances the num-parsed attribute for the given item. If 'error?' is true, then the item will be tagged as containing an error if it hasn't been already.

The function returns [new-eitem, new?], where 'new-eitem' is the advanced version of 'item', and 'new?' is true if this item did not previously exist.
```
defn advance-memoized (item:EItem, error?:True|False) -> [EItem, True|False]
```

The function uses 'advance-table', and hence 'advance-table' must be cleared at the start of processing each Earley set.

### Processing a Single Earley Set ###
@[earley search algorithm process-set]

Processes the current Earley set, and adds scanned item to the next Earley set.
```
defn process-set (set-index:Int,
                  current-set:ESet,
                  next-set:ESet,
                  next-input:SExpToken,
                  include-all-rules?:True|False,
                  error-recovery?:True|False) -> False
```

- set-index is the index of the Earley set.
- current-set is the current Earley set to process.
- next-set is the next Earley set to add scanned items.
- next-input is the upcoming input token.
- include-all-rules? is true if prediction should include all possible rules
  for a given production. If false, prediction should use look-ahead to include
  only rules with matching first-sets.
- error-recovery? is true if process-set is being ran after performing error recovery,
  and thus any scanned items should be tagged as containing errors.
  
For each item in the current set, we look at its upcoming token. There are three cases:
Case: A terminal is upcoming
Case: A production is upcoming
Case: A rule is (non-null) completed.

#### Case: A terminal is upcoming ####
Test whether the given terminal matches against the given input. If it does, then advance the item, and add it to the next set if it hasn't been added already.

#### Case: A production is upcoming ####
First check whether we should perform a null-advance. If the production is nullable, then advance the item, and add it to the current set.

Then add predicted rules for the upcoming production to the current set if we have not already added predicted rules for this production.

####  Case: A rule is non-null completed ####
Iterate through all the items in the parent set that have the completed production as their upcoming production. Advance these items and add them to the current set.

For items with right-recursive links, we need to advance their roots instead, and also add a new chain completion to the advanced roots.

### Commiting an Earley Set ###
@[earley search algorithm commit-set]

Commits the set 'eset' to the algorithm state.
```
defn commit-set (set-index:Int, eset:ESet) -> False
```
The function does the following things:
- Adds items to the setlist.
- Adds items to the completion list.
- Adds scanned terminals to the terminal set.
- Records new ends to items.
- Computes the completion roots for items.

### Main Algorithm ###
@[earley search algorithm process-all-sets]

This function processes all Earley sets. After it is completed,
any errors will be added to the missing vector.
```
defn process-all-sets () -> False
```

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
This case occurs when we have finished parsing. In this case, we just commit the current set and finish the algorithm.

#### Case B: Error Recovery ####
This case occurs when the parser is stuck and is awaiting a terminal that does not match the input stream.

First, we want to save the context that caused this error. So compute the full list of Earley items (without using predictive look-ahead), and save this information.

There are three cases we can be in, and the error recovery action to take.
- An item is expecting the end of a list: Drop input.
- An item is expecting the start of a list: Insert a list.
- An item is expecting a terminal that matches a wildcard: Insert a wildcard.

In terms of priority, we want to take actions with the following priority:
1. Drop input
2. Insert wildcard
3. Insert list

#### Case C: Standard Iteration ####
This case occurs when a normal iteration is completed. We need to commit the current set, advance the input stream, and then go to the next iteration. To advance the input stream we either advance to the end of the list if the rest terminal was scanned, or by a single form otherwise.

============================================================

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


## Explanation of Parsing Tables ##

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

### Custom Error Messages ###

After pruning non-descriptive items, all remaining items have the form:

```
(rule 4) [CS = C • X BS, S15]
```

where there is a clear upcoming production or terminal that does not match against the next input. Currently, an auto-generated error message is created from this item. Instead, we can provide custom control to the user to write a descriptive error message given this context.

## Keyword Sets ##

The following concepts need to be supported by the parsing system to allow for natural language extensions:

Concept of Keyword Set: A keyword set is a set of symbols that are reserved as "keywords" in the language. Language extensions can freely add additional keywords to this set to grow its vocabulary.

Concept of Identifer: An identifier is commonly defined as any symbol that is *not* a keyword. Because the set of keywords can be user-extended by the user, this definition of an identifier needs to updated consistently as well.

Concept of Keyword Group: It is common for a given language extension to define its own set of keywords, in its own separately named group, e.g. `jitx-keywords`. The user then needs the ability

- to add this group of keywords, en-masse, to another set of keywords, and
- to remove this group of keywords, from the allowed symbols that make up an identifier.

# Adding Support for Factored Productions #

Consider the following rule definitions:

  E = A B
    | A C
    | A D

The key proposal is to add the mechanism required to be able to transform the above into this:

  E = A X
  X = B
    | C
    | D

This leads to drastically reduced earley sets for grammars containing many operators.

The transformation to the grammar is, itself, quite straightforward. The difficulty is in consistently treating priority, associativity, and evaluation.

## Parse Node Selection ##

In full generality, the metrics we use to compare ParseNodes are:
- length
- priority
- associativity
- associativity-length
- order

The parse node selection can be thought of as being split into two stages:

Stage 1: Given E [0 to 100], what rule do we use to parse E? Since length is known, this decision will be made on the metrics:
  - priority
  - associativity
  - associativity-length
  - order

Stage 2: Given E [0 to 100] = E + E, what are the lengths of the subproductions? This is decided based upon the associativity of the rule. If left-associative or non-associative, then we want to greedily choose the longest productions from left-to-right. If right-associative, then we want to greedily choose the longest productions from right-to-left.

Look at stage 1 first, for factored nodes:
- To compare priority, we can find the set of priorities obtained by the factored production, and use the maximum.


Step 1: Consider a rule, S [0 to 100] = A B C D E, left-associative with no factoring. Now we want to traverse left-to-right and determine lengths of subproductions. This can be done without considering whether they are factored or not, since the longer the better.

Step 2: For each of the subproductions, we now know their lengths, but we don't know which rule to use. And these subproductions may be factored.

Given A is from [0 to 50], and there exists one rule like this:
  Case: A [0 to 50] = X Y (factored on X)
  Case: A [0 to 50] = X Y (factored on Y)

  Desire: Find the X with the maximum priority such that A parses. Thus, list X's ends/starts sorted by priority, then associativity.


Currently: Given E [0 to 100], I can calculate the right rule to use given only the list of rules. Priority and Associativity are easy. Associativity-Length requires computing lengths of the subrules.

Next: Suppose E is left-factored. Thus we don't know the end. We can ask the left-factored production to try giving us candidate ends (sorted by priority).

Let's first tackle priority:
  - We can get the highest priority parse node, this is possible.
  - We can get the highest priority and associativity parse node.
  - If associativity is Non-Associative, we can get the order as well.
  - Associativity-Length is harder.

Suppose we finally know the associativity. Then we would know what direction to parse these nodes in. Now, we have no choice. We have to determine the nodes up to length at least. And then we can select from them.


# TODO #

- Documentation and Cleanup
- Checks for Infinite Recursion
- More robust transformations for left-right-ambiguity elimination
- Grammar analysis for keyword productions (and negated productions)
- Prevent infinite error recovery
- Improve auto-generated error messages during error message generation
- Custom error messages
- Language transformation and Features
