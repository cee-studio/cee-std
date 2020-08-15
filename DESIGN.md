# Design Goals and Principles

## Goals
*  Easy to maniupate strings
*  Ready to use vector, set, map, stack, and dict

## Principles

*  Standard C functions should be directly appliable to memory layout equivalent cee structs,
   e.g. all C standard string functions can be applied directly to `cee_str *`.
   
*  Easy to build correct Proof of Concept (POC) by using the default settings,
   e.g. memory leaks are considered benign in POC.

*  Memory leak removal is considered as an optimization, it should be
   handled easily in later iterations with advanced tooling like 
   https://cee.studio, valgrind, or ASAN.

*  Performance optimziation should be easy to do by removing abstraction and 
   falling back to more C idiomatic implementations.


## Other considerations
* struct cee_* are used intentionally to distinguish them from their memory layout 
  equivalent data structures, but they need to be handled different in memory deallocation

* Code readability triumphs conciseness with the assumption morden IDE's 
  auto-completion will be used to write code

* More concise abstraction should be handled by libraries built on top of this lib