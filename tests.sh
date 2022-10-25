dir=$(realpath --relative-to=. "$(dirname "$(readlink -f "$0")")")
cmake --build ./build --config Debug || exit $?
function try {
  echo " - $file"
  out=$($dir/build/fy $args)
  exit=$?
  echo "$out"
  if [ "$exit" -ne 0 ]; then
    echo " - $file failed with exit code $exit"
    if [ $DEBUG ]; then
      lldb -- $dir/build/fy $args;
    fi
    exit 1
  else
    echo " - $file passed"
  fi
}
for file in $dir/examples/*.fy
do
  args="run examples/$(basename $file)"
  try
done
echo " - Examples test done"
for file in $dir/lib/*.fy $dir/lib/**/*.fy
do
  file=${file##$dir/lib/}
  file=${file%.fy}
  args="com lib/$file /dev/null"
  try
done
echo " - All lib files compile"
for file in $dir/tests/*.fy $dir/tests/**/*.fy
do
  file=${file##$dir/tests/}
  file=${file%.fy}
  export QUIET=1
  args="run tests/$file.fy 2>&1"
  try
  if [ -f "tests/$file.txt" ]; then
    expected=$(<"tests/$file.txt")
    if [ "$out" != "$expected" ]; then
      echo "Wrong output for $file, expected '$expected', got '$out'"
      exit 1
    fi
  fi
done
tput setaf 2
echo " - All tests passed"
tput sgr0