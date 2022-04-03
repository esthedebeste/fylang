dir=$(realpath --relative-to=. "$(dirname "$(readlink -f "$0")")")
if [ "$1" = "-F" ]; then shift; else 
  clang++ $dir/src/main.cpp -o $dir/bin/fylang -lLLVM-15 -lstdc++ -fuse-ld=lld || exit $?
  echo "
 - Compiled fylang"
fi
if [ "$1" = "-Q" ]; then 
  export QUIET=1
  shift
fi
if [ "$1" = "-U" ]; then 
  export NO_UCR=1
  shift
fi
name=$1
bin_name=${name##**/}
bin_name=${bin_name%.fy}

mkdir $dir/bin -p
shift
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