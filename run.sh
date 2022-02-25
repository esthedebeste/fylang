mkdir bin -p
rm -rf bin/*
cd ./bin
clang++ -c ../src/fy-std.cpp -o ./fy-std.o
echo " - Compiled fy-std"
clang++ ../src/main.cpp -o ./fylang  -lLLVM -lstdc++ -fuse-ld=lld &&
echo "
 - Compiled fylang" &&
./fylang ../hello.fy ./hello.ll &&
echo "
 - Compiled hello.fy" &&
# Link and compile hello.ll with LLVM and c std
clang ./hello.ll ./fy-std.o -o ./hello -lLLVM -lstdc++ -fuse-ld=lld &&
echo "
 - Linked hello.ll" &&
./hello
echo "
 - Executed hello with exit code $?"
cd ..