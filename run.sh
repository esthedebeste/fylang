dir=$(realpath --relative-to=. "$(dirname "$(readlink -f "$0")")")
if [ "$1" = "-F" ]; then shift; else 
  clang++ $dir/src/main.cpp -o $dir/bin/fylang -lLLVM-15 -fuse-ld=lld -std=c++17 || exit $?
  echo "
 - Compiled fylang"
fi
if [ "$1" = "-D" ]; then 
  export DEBUG=1
  shift
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
$dir/bin/fylang run $name $dir/bin/$bin_name.ll
exit $?