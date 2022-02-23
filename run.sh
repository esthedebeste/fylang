mkdir bin -p
rm -rf bin/*
cd ./bin
clang++ -c ../src/fy-std.cpp -o ./fy-std.o -fuse-ld=lld 
echo " - Compiled fy-std"
clang++ ../src/main.cpp -o ./fylang.ll -emit-llvm -S &&
echo "
 - Compiled" &&
clang ./fylang.ll -o ./fylang -lLLVM -lstdc++ -fuse-ld=lld &&
echo "
 - Linked" &&
./fylang ../hello.fy ./hello.ll &&
echo "
 - Executed" &&
clang ./hello.ll ./fy-std.o -o ./hello -lLLVM -lstdc++ -fuse-ld=lld &&
echo "
 - Linked" &&
./hello
echo "
 - Executed with exit code $?"
cd ..