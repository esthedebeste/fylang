dir=$(dirname "$(readlink -f "$0")")
for file in $dir/examples/*.fy
do
  $dir/run.sh "examples/$(basename $file)" || exit $?
done

clang++ $dir/src/main.cpp -o $dir/bin/fylang -lLLVM-15 -lstdc++ -fuse-ld=lld || exit $?
for file in $dir/lib/**/*.fy
do
  $dir/bin/fylang ${file##**/lib/} /dev/null || exit $?
done