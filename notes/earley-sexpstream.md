# SExpTokens #
@[SExpToken Definition]

## Standard SExp Forms ##
```
SExpForm :
  form
  list:List
```

This represents a single s-expression form. For implementing matching against list rest, we also provide all subsequent forms in the `list` field.

## End of List ##
```
SExpListEnd
```

This represents the end of a list in the input.

## Wildcards ##
```
SExpWildcard
```

This represents an inserted wildcard, able to match against any terminal except for list start and list end.

## End of Input ##
```
EndOfInput
```

This represents the end of the input.

# SExpStream #

A `SExpStream` represents the input list of tokens. It is constructed from a single s-expression list.

## Retrieving the Next Token ##

The `peek` and `info` functions return the upcoming token and its file information.

```
defmulti peek (s:SExpStream) -> SExpToken
```

```
defmulti info (s:SExpStream) -> FileInfo|False
```

Remember that all `SExpStream` objects end with a `SExpListEnd` followed by an `EndOfInput` token.

## Advancing the Stream ##

The `advance` function steps the stream past the next upcoming token. If `expand-list?` is passed true, then the stream recursively enters the next upcoming list (if it is a list).

```
defmulti advance (s:SExpStream, expand-list?:True|False) -> False
```

The `advance-rest` function steps past all tokens at the current list-level, ending just before `SExpListEnd`. Thus `peek` should return `SExpListEnd` after calling `advance-rest`.

```
defmulti advance-rest (s:SExpStream) -> False
```

It is an error to advance the stream when the upcoming token is `EndOfInput`.

## Error Recovery Using Wildcards ##

Wildcards match against any terminal except for ListStart and ListEnd, so the stream allows us to insert a wildcard token to the beginning of the stream.

```
defmulti insert-wildcard (s:SExpStream) -> False
```

The stream allows us to also insert an empty list to the beginning of the stream, to recover from if the parser is stuck awaiting a ListStart.

```
defmulti insert-list (s:SExpStream) -> False
```

It is an error to insert any tokens into the stream if the upcoming token in `EndOfInput`.

## Retrieve Results ##

All advanced items are added to a vector to retrieve the final flat list of tokens. 

This returns the flat list of tokens seen by the stream.
```
defmulti inputlist (s:SExpStream) -> Vector<SExpToken>
```

This returns the corresponding file information for each token seen in the stream.
```
defmulti infolist (s:SExpStream) -> Vector<FileInfo|False>
```
