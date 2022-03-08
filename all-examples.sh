dir=$(dirname "$(readlink -f "$0")")
for file in $dir/examples/*
do
  $dir/run.sh "examples/$(basename $file)" || exit $?
done
