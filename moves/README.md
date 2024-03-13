# C++ moves, constructors and rule of 3/5/0

The aim of this was for me to revisit how moves are implemented in C++
in detail, as well as the rules of 3/5/0.

The rules of 3/5 are applied when an object owns (a) resource(s) (otherwise
the rule of 0 applies). 5 overloads are needed when an object should
support moves.

Moves need the following:

- Such objects need to be option-like (a moved-from object needs to
  represent a "null-like" state so that when the destructor runs, it
  can skip running any action). (C++ follows a run-time move semantics
  approach, hence needs runtime information for this.)

- Need both moving `operator=` and constructor overloads

- The `oberator=` overload needs to destruct the object at the target
  location explicitly (via `destruct` method in this example, see
  first todo point on open question about the how), because we're just
  going to fill it with the new data, C++ doesn't call the destructor
  on it at that time.

- Can/should assert at runtime that an object is only moved-from once.

- Need to transfer all fields explicitly in the custom
  implementations, and not to forget to do so with `std::move`.

## Random details

- In copy operations, obviously copy data *before* mutating it if the
  mutation concerns the target object.

- Assignment overloads should return a (normal) reference to the
  assigned-to location (`*this`) for chaining (e.g. `auto a = b = c;`).

## Todo

* I haven't verified yet why, on move assignment, the destructor of
  the original target is not called, yet its string seems to be freed
  (can't call the real destructor, `~Foo`, explicitly). Might be because on
  string assignment, string's move assignment overload does it.
  
  Does C++ call destructors on individual fields after running my `~Foo`?

* When an object doesn't need to support moves, but owns resources, it
  should implement move overloads anyway and make them private? Or
  what is the solution to prevent accidental copies?

* When an object should support moves and owns resources, but doesn't
  need to do anything than have all fields moved, does it still need
  to implement 5, or 0 since the default implementations are fine?

* Read [What is the copy-and-swap idiom](https://stackoverflow.com/questions/3279543/what-is-the-copy-and-swap-idiom)
  on the topic of "avoiding code duplication, and providing a strong
  exception guarantee" (another solution to the `destruct` method
  currently used?)

* Factor out the move operations without using macros.
