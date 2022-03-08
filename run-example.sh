mkdir bin -p
rm -rf bin/*
cd ./bin
name=$1
shift
clang++ -c ../src/fy-std.cpp -o ./fy-std.ll -S -emit-llvm
clang++ -c ./fy-std.ll -o ./fy-std.o
echo " - Compiled fy-std"
clang++ ../src/main.cpp -o ./fylang -lLLVM-15 -lstdc++ -fuse-ld=lld &&
echo "
 - Compiled fylang" &&
./fylang ../examples/$name.fy ./$name.ll &&
echo "
 - Compiled $name.fy" &&
# Link and compile $1.ll with LLVM and c std
clang ./$name.ll -o ./$name-optimized.ll -O3 -S -emit-llvm &&
clang ./$name-optimized.ll ./fy-std.o -o ./$name -lLLVM-15 -lstdc++ -fuse-ld=lld -g &&
echo "
 - Linked $name.ll,
 - Running $name" &&
./$name "$@" 
exitc=$?
echo "
 - Executed examples/$name.fy with exit code $exitc"
cd ..
exit $exitc