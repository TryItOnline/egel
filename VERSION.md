VERSION
=======

A list of all major changes between versions of the interpreter.

## v0.0.3 -- it may all break down yet

+ bugfix on '(1)' -length one array- results
+ switch to '#' style comments
+ regular expression support
+ small performance tweaks
+ got rid of lijero's K combinator solution for statements

## v0.0.2 -- why bother?

+ check for overflow on simple math operators
+ changed from dot to colon syntax to free up '.' as a combinator
+ added semicolons as syntactic sugar to the language

## v0.0.1 -- heap of dung

+ CHANGED TO STANDARD PATTERN MATCHING SYNTAX
+ provisional fix to always print a dot in floats
+ fixed a scope bug in let
+ added hexadecimal numbers and binary operators
+ fixed a bug with lexical scope and try/catch blocks

## v0.0.0 -- not even wrong

This is the version where a plethora of bugs were squashed, technically
most things are in place, but there are still a large number of loose
ends.

+ Symbolic rewriting
+ REPL and batch mode support
+ Initial support for a prelude and basic libraries
+ Experiments with mutable state and OO
+ Concurrency
+ Dynamic libraries
+ Optional system-wide install

## v0.0 -- initial release

The initial commit. Expect lots and lots of bugs!

