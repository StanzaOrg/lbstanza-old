# Earley Search Procedure #

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

#### Recording new Ends ####
We record new ends for all items when the dot is neither at the beginning of the item, nor at the end of the item.

The ends are relative to the starting position of the item. 

#### Computing the Completion Root ####

Compute the completion-root for each deterministic-reduction in the set.

A deterministic reduction looks like this:
```
(rule 0) [Start = X X X • AS, S0]
```
It has one remaining production to parse, and this item is the only item with that upcoming production.

The completion of a deterministic reduction is calculated like so:
First search for a completion-root in the parent. Let `pitem` be the first item for production `Start` started at position 0. If `pitem` has a completion-root, then this item is a right-recursive link, and `pitem` gives us the completion root for the item. If no completion-root is found in the parent, then this item is a right-recursive root.

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
