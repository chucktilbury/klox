# KLOX
A modified implementation of clox, which was created by Robert Nystrom.
You can find his blog here: http://journal.stuffwithstuff.com/
You can find his book here: https://craftinginterpreters.com/

## The Plan
1. Do most of the modifications listed below. 
2. Choose a GUI framework to bind to the result. 
3. Build a GUI source level debugger.
4. Complete a simple IDE base on the debugger. 
5. Go nuts (if I haven't already).

## Random Notes 
I made these notes while I was going through the book and copying the source code. They may or may not make sense. I am in the very early stages of getting things up and running. The idea is to build a stable and fast language that I would actually like to use. One that is adapted to my preferences, rather than adapting my preferences to the tools at hand. I am mostly interested in writing GUI based stuff right now, so that's where this is heading. 

--------------------
Actual questions
----------------------
* How does expression parser actually work?
* How does the code for upvalues actually work?
* Why are "this" and "super" not normal literal tokens?

----------------
Changes planned
---------------
* Line numbers in the chunk data structure. (ch 14 #1)
* Add instructions for !=, <=, and >=
* Add signed and unsigned integer types to the interpreter. Also implement the
  promotion rules.
* do the "challenges" for chapter 19
* Chapter 21 challenges
* Fix the REPL with a proper readline implementation.
* Chapter 22: Fix This: The next step is figuring out how the compiler
    gets at this state. If we were principled engineers, we’d give each
    function in the front end a parameter that accepts a pointer to a
    Compiler. We’d create a Compiler at the beginning and carefully thread
    it through each function call but that would mean a lot of boring changes
    to the code we already wrote, so here’s a global variable instead:
* Allow CLOX to handle more than 256 locals.
* Allow CLOX to handle more than 256 globals
    * Change the index to 2 bytes and then make the stack behave.
* Add a "const" keyword and implement it.

* Add arrays and dicts as first-class objects. Note that dict keys can be of
  any particular type, as can array and dict elements, including classes
  instances.

* Add string interpolation. That is when a {varname} is encountered, it is
  automagically converted and substituted in the string.

* The maximum number of instructions that an instruction can encode is 16
  bits. There should be a "long jump" instruction that can handle more bits.
  The parser will have to backpatch the instruction to implement that. (see
  chapter 23)

* Add support for switch/case.

* Add support for continue/break

* Standard library and math library.

* Checking return values for errors should be done in LOX code, not by the VM,
  as the book suggests.

* Challenge #2 from chapter 24 (big)

* Stack traces need to print more information, such as the names of functions
  and their line number. Some way to track the parameter values or something
  would also be nice.

* Implement exceptions. (big one)

* Support partial statements in REPL, such that it does not barf on a function
  definition.

* Chapter 25 challenge #1. Look into performance improvements.
* Chapter 25 challenge #2. Understand what this is trying to say.

* Chapter 26 challenge #2 answer.

* Chapter 27 challenges, especially #4. Instead of accessing a variable by
  name, access it by hash.... somehow....

* Break up the compiler into component parts. Inline things that can be
  inlined with macros where possible.

* Change the less token to a colon in classDeclaration(). Support it in the
  scanner.

------------------------------------
Changes actually implemented
--------------------------------------

* Chapter 24 challenge #1, make IP more efficient.

----------------------------------
Steps to add an object type
------------------------------------
* Create the data structure in object.h.
* Add the object type enum  in object.h.
* Create the IS_XXX and AS_XXX macros in object.h.
* Create the function that creates the object in object.c.
* Add the code in memory.c to freeObject() to free it.
* Add the code in memory.c to blackenObject() to black it.
* Add the code in object.c to print it.
* This does not include the code to parse anything.
