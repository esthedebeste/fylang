## VTABLES FOR CLASSES
 - instance struct has pointers to the vtables it implements 
 - these vtables contain pointers to the actual functions to run (with the class/struct as the first argument (basically rust's "&mut self"))
 - https://mapping-high-level-constructs-to-llvm-ir.readthedocs.io/en/latest/object-oriented-constructs/virtual-methods.html
 - https://alschwalm.com/blog/static/2017/03/07/exploring-dynamic-dispatch-in-rust/
 - https://doc.rust-lang.org/1.30.0/book/first-edition/trait-objects.html#representation
 - ![CPP](https://alschwalm.com/blog/static/content/images/2017/03/cat_and_clone_cpp-1.png)
 - ![RUST](https://alschwalm.com/blog/static/content/images/2017/03/clone_mammal_rust-1.png)