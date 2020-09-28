# Earley Parser #

@[file:earley-search.md] Explains the Earley search procedure.
@[file:earley-sexpstream.md] Explains the `SExpStream` utility for controlling the input stream.



============================================================



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
