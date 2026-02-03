



# Important Questions
## What can cross binary boundaries?
Only primitives, explicitly versioned POD structs, opaque handles, and borrowed pointers with documented lifetimes may cross binary boundaries. No ownership, exceptions, or STL types may cross.