dir=$(realpath --relative-to=. "$(dirname "$(readlink -f "$0")")")
clang++ $dir/src/main.cpp -o $dir/bin/fylang -lLLVM-15 -std=c++17 -fuse-ld=lld || exit $?
for file in $dir/examples/*.fy
do
  $dir/bin/fylang run "examples/$(basename $file)" || exit $?
done
echo " - Examples test done"
for file in $dir/lib/**/*.fy
do
  $dir/bin/fylang com ${file##$dir/lib/} /dev/null || exit $?
done
echo " - All lib files compile"
for file in $dir/tests/*.fy $dir/tests/**/*.fy
do
  file=${file##$dir/tests/}
  file=${file%.fy}
  out=$(QUIET=1 $dir/bin/fylang run "tests/$file" 2>&1)
  if [ "$?" -ne 0 ]
  then
    echo " - $file failed"
    echo "$out"
    exit 1
  fi
  if [ -f "tests/$file.txt" ]; then
    expected=$(<"tests/$file.txt")
  fi
  echo $out
  if [ "$out" != "$expected" ]; then
    echo "Wrong output, expected '$expected', got '$out'"
    exit 1
  fi
done
echo " - All tests passed"