# Earley Parse Tree #

# ParsedProd #
Represents a parsed production. It does not specify the rule that the production was parsed with.
```
defstruct ParsedProd :
  prod: Int
  start: Int
  end: Int
```
- prod is the identifier of the production that was parsed.
- start is the starting position of the production that was parsed.
- end is the ending position of the production that was parsed.

# ParseForest #
@[Earley parse forest]

A `ParseForest` represents an easily queried representation of all possible parses of an input stream.

## Creation ##
Create a new ParseForest given the completed items and terminal set.
```
defn ParseForest (eitems:ECompletionList,
                  tset:TerminalSet) -> ParseForest
```

## Parse Items ##
Returns the list of completed Earley items corresponding to a parsed production.
```
defmulti items (f:ParseForest,
                prod:ParsedProd) -> List<EItem>
```

## Scanned Terminals ##
Returns true if the given terminal `term` matched the input stream at position `position`. 
```
defmulti match? (f:ParseForest,
                 term:GTerminal,
                 position:Int) -> True|False
```                 
                
## Parse Positions ##
Returns the possible ending positions for a given token.
```
defmulti ends (f:ParseForest,
               item:EItem,
               item-end:Int,
               token-index:Int,
               token-start:Int) -> Seqable<Int>
```               
For the item 'item' ending at 'item-end', this computes the ending positions for the token at index 'token-index' starting at position 'token-start'. 
Ends are returned in descending order.

Returns the possible starting positions for a given token.
```
defmulti starts (f:ParseForest,
                 item:EItem,
                 item-end:Int,
                 token-index:Int,
                 token-end:Int) -> Seqable<Int>
```                 
For the item 'item' ending at 'item-end', this computes the starting positions for the token at index 'token-index' ending at position 'token-end'. 
Starts are returned in ascending order.

