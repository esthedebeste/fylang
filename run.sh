mkdir bin -p
rm -rf bin/*
cd ./bin
name=$1
shift
clang++ -c ../src/fy-std.cpp -o ./fy-std.o
echo " - Compiled fy-std"
clang++ ../src/main.cpp -o ./fylang  -lLLVM -lstdc++ -fuse-ld=lld &&
echo "
 - Compiled fylang" &&
./fylang ../$name.fy ./$name.ll &&
echo "
 - Compiled $name.fy" &&
# Link and compile $1.ll with LLVM and c std
clang ./$name.ll ./fy-std.o -o ./$name -lLLVM -lstdc++ -fuse-ld=lld &&
echo "
 - Linked $name.ll,
 - Running $name" &&
./$name "$@" 
echo "
 - Executed hello with exit code $?"
cd ..