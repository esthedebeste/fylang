## Garbage Collection // Reference Counting // Smart Pointers
 - https://llvm.org/docs/GarbageCollection.html
    - does not help at all i do not know what this even describes
- Get rid of non-smart pointers? I kinda want fylang to be a more high-level language, and not have to deal with things like free, 
    - Reference counting pointer is basically just a pointer to a struct{refs: uint_ptrsize, data: T}
        - Seperate strong and weak refs??? probably not necessary yet.
        - Other idea: have the "new" keyword automatically create a RC pointer. In a context where you'd use "new" you probably wouldn't have any need for the real memory address. (AKA. don't do unsafe mallocs as a language feature.)
 - Definitely not a fan of the current `__free__()`  strategy. Should 100% be replaced with smth like `__finalize__()` which would be called before the smart pointer is freed. 
 - Might have to change "gen_ptr" usage to "gen_store" and "gen_load".
    - Difference betwen gen_val and gen_load?

## VTABLES FOR CLASSES
 - instance struct has pointers to the vtables it implements 
 - these vtables contain pointers to the actual functions to run (with the class/struct as the first argument (basically rust's "&mut self"))
 - https://mapping-high-level-constructs-to-llvm-ir.readthedocs.io/en/latest/object-oriented-constructs/virtual-methods.html
 - https://alschwalm.com/blog/static/2017/03/07/exploring-dynamic-dispatch-in-rust/
 - https://doc.rust-lang.org/1.30.0/book/first-edition/trait-objects.html#representation
 - ![CPP](https://alschwalm.com/blog/static/content/images/2017/03/cat_and_clone_cpp-1.png)
 - ![RUST](https://alschwalm.com/blog/static/content/images/2017/03/clone_mammal_rust-1.png)