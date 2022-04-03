dir=$(realpath --relative-to=. "$(dirname "$(readlink -f "$0")")")
clang++ $dir/src/main.cpp -o $dir/bin/fylang -lLLVM-15 -lstdc++ -fuse-ld=lld || exit $?
function get_exec() {
  b=${1##**/}
  QUIET=1 $dir/bin/fylang $1 $dir/bin/$b.ll || exit $?
  clang $dir/bin/$b.ll  -o $dir/bin/$b -O3 -lLLVM-15 -lstdc++ -fuse-ld=lld -g || exit $?
  executable="$dir/bin/$b"
}
for file in $dir/examples/*.fy
do
  $dir/run.sh -F -Q "examples/$(basename $file)" || exit $?
done
echo " - Examples test done"
for file in $dir/lib/**/*.fy
do
  $dir/bin/fylang ${file##$dir/lib/} /dev/null || exit $?
done
echo " - All lib files compile"
for file in $dir/tests/*.fy $dir/tests/**/*.fy
do
  file=${file##$dir/tests/}
  file=${file%.fy}
  get_exec "tests/$file"
  out=$($executable 2>&1)
  if [ "$?" -ne 0 ]
  then
    echo " - $file failed"
    echo "$out"
    exit 1
  fi
  expected=$(cat "tests/$file.txt")
  echo $out
  if [ "$out" != "$expected" ]; then
    echo "Wrong output, expected '$expected', got '$out'"
    exit 1
  fi
done
echo " - All tests passed"