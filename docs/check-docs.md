# Stanza's Document Consistency Checker #

Stanza's document consistency checker is a simple tool that helps you to organize your repository comments and documentation.

The tool helps to solve two problems:

1. It forces you to describe the purpose of all files in the repository, how they are organized, and how they relate to each other.

2. It automatically detects when comments are out-of-date. 

And the tool accomplishes this using two techniques:

1. A root document: All files must be described (either directly or indirectly) by the root document. 

2. Hashes: When documents refer to specific snippets in the codebase, the tool computes a hash of the snippet at the time the document was written. When the code snippet changes, then the tool asks you to update its corresponding documentation.

# Running the Consistency Checker #

Given a properly formatted `.doc` file, say `notes.doc`, you can run the consistency checker using this command:
```
stanza check-docs notes.doc
```

# Basic Mechanisms #

## The .doc File ##

A repository contains a single `.doc` file to tell the tool about how the repository is organized. Here is an example of a minimal doc file:

In `notes.doc`:
```
root:
  "notes/index.txt"
files:
  "notes"
  "src"
```

This tells the tool that:
1. The root document is `notes/index.txt`.
2. All files in the `notes` and `src` folders should be described by the root document.

## Document Files ##

A document file can be any plain-text file. The tool looks for special annotations in the document file. Here are two examples of document files:

In `notes/index.txt`:
```
These documents describe the EARTH system. 

@[file:usage.md] This document explains how to use the EARTH system.
@[file:license.txt] This describes the license of the EARTH system.
@[file:../src/src-organization.md] This describes how the soure code is organized.
```

In `src/src-organization.md`:
```
The source code of EARTH is organized in the following way:

@[file:input-ir.stanza] This converts the textual description of a planet into the EARTH internal datastructure.
@[file:mid-level.stanza] This processes the EARTH internal datastructure until it is ready for export.
@[file:export.stanza] This outputs the EARTH datastructure in a human-readable way. 
```

Document files can reference other document files, and the purpose is to provide a clear description of the project goals and organization. 

The root document is the starting document for the entire repository. Through this root document, every other file in the repository will be eventually described. 

### Referencing Other Files ###

The annotation:
```
@[file:myfile.txt] 
```
is a reference to another file in the repository. This path is relative to the document that it appears in.

The tool will issue an error if there is a file in the repository that is not eventually referenced through the root document. 

## Referencing Code Snippets ##

The tool automatically checks when code changes so that you can update your documentation. 

### Example Code and Documentation ###

Suppose we have the following code file and document file:

In `src/input-ir.stanza`:
```
defpackage earth/input-ir :
  ...

;+[EARTH available commands]
public defn read-commands (s:String) -> CommandIR :
  switch(s) :
    "make-earth" : asdf
    "delete-earth" : asdf
    else : ...
;/[EARTH available commands]
```

In `notes/usage.md` :
```
@[EARTH available commands]

Here are the available commands in EARTH:
make-earth: This creates a new Earth model.
delete-earth: This deletes the current Earth model.
```

The `usage.md` file explains the available commands that a user can call to use the EARTH program. The `read-commands` function in `input-ir.stanza` implements the behaviour of all the available commands. 

### Referencing Code Snippet Annotations ###

The annotation `@[EARTH available commands]` tells the tool that this written section depends upon the code snippet called "EARTH available commands". 

The annotations `+[EARTH available commands]` and `/[EARTH available commands]` tells the tool where the code snippet called "EARTH available commands" starts and ends. If the ending tag `/[EARTH available commands]` is omitted, then the tool assumes that the snippet ends at the start of the next code block with the same indentation level. 

If the code that makes up "EARTH available commands" changes, the tool will ask you to update the corresponding documentation as well. 

### Computed Hashstamps ###

After writing a document file, the first time that you run `check-docs`, the system will compute and add hashstamps to all the document files in the repository. The `notes/usage.md` example will be updated to this:

In `notes/usage.md` :
```
@[EARTH available commands]<863A3B18>

Here are the available commands in EARTH:
make-earth: This creates a new Earth model.
delete-earth: This deletes the current Earth model.
```

This hashstamp, `<863A3B18>`, is a short description of the contents of the "Earth available commands" code snippet.

### Outdated References ###

If the implementation of the available EARTH commands change, the tool will report an error like the following:
```
notes/usage.md:43.0: Reference to changed block "EARTH available commands" is now out-of-date.
```

And the reference in `notes/usage.md` will be changed to:
```
@[EARTH available commands]<outdated:F15F75A0>

Here are the available commands in EARTH:
make-earth: This creates a new Earth model.
delete-earth: This deletes the current Earth model.
```

You should manually review `usage.md` at this point, and update the documentation. Once you are satisfied, you can remove the `outdated` tag to indicate that it is now up-to-date. 

# Interaction with Version Control Systems #

The document consistency checker is designed to interact nicely with an existing version control system. 

When code changes, its corresponding hashstamps will be changed as well in the repository documentation. You can then review the updated documentation to read about any changed behaviour. If the text of the documentation has not changed, and only the hashstamp has changed, then this can be taken as an explicit signal from the code's author that the change does not affect its documented behaviour. 

# Inspiration #

Stanza's document consistency checker is inspired by the notes system used in the Glasgow Haskell Compiler (GHC), a simple system for decluttering comments by moving descriptions to separate "notes".
