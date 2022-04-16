subcmd="run"
build_dir=$(realpath --relative-to=. "$(dirname "$(readlink -f "$0")")/build")
cmake --build $build_dir --config Debug
while getopts "duc" opt; do
    case "$opt" in
        d) export DEBUG=1;;
        u) export NO_UCR=1;;
        c) subcmd="com";;
    esac
done
shift $((OPTIND-1))
fy="$build_dir/fy"
cmd="$fy $subcmd $@"
QUIET=1 $cmd
exit=$?
if [ "$exit" -ne 0 ]; then
    tput setaf 1
    echo "[run.sh] Executed with exit code $exit"
    if [ $DEBUG ]; then
        lldb $cmd
    fi
    exit 1
fi
tput setaf 2
echo "
[run.sh] Executed with exit code 0"
tput sgr0
exit $exit