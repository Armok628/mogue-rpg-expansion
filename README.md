# mogue2
An expansion of an old roguelike of mine to add RPG elements

There are bad programming practices everywhere in here.
For an example of what I think is (more or less) good technique, see my LISP dialect, "csl".

To-do:
* Separate like functions into different files
* Place prototypes, etc. in header files
* Abstract areas with too much indentation

Ideas for another "sequel":
* Creatures with function pointer arrays (as spells)
  * How to store spell names?
  * Macro for creature (type or individual) instantiation?
* Scrolling screen
  * Optimizations?
* World map with persistent areas
  * Should events happen outside current area?
* Construction of walls, etc.
* Inventory
* Saving/Loading
  * What format to save data in?
* Gameplay that isn't boring
  * Better magic (not just an invisible stat change)
