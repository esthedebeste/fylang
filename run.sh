name=$1
bin_name=${name##**/}
bin_name=${bin_name%.fy}
dir=$(realpath --relative-to=. "$(dirname "$(readlink -f "$0")")")

mkdir $dir/bin -p
shift
clang++ $dir/src/main.cpp -o $dir/bin/fylang -lLLVM-15 -lstdc++ -fuse-ld=lld &&
echo "
 - Compiled fylang" &&
$dir/bin/fylang $name $dir/bin/$bin_name.ll &&
echo "
 - Compiled $name" &&
# Link and compile $1.ll with LLVM and c std
clang $dir/bin/$bin_name.ll -o $dir/bin/$bin_name-optimized.ll -O3 -S -emit-llvm &&
clang $dir/bin/$bin_name-optimized.ll -o $dir/bin/$bin_name -lLLVM-15 -lstdc++ -fuse-ld=lld -g &&
echo "
 - Linked $bin_name.ll,
 - Running $bin_name" &&
$dir/bin/$bin_name "$@" 
exitc=$?
echo "
 - Executed $name with exit code $exitc"
exit $exitc