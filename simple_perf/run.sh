export SYSTEMC_HOME=$HOME/opt/systemc-2.3.4

if [ "$(uname -s)" = "Linux" ]; then
    export LD_LIBRARY_PATH=$SYSTEMC_HOME/lib:$LD_LIBRARY_PATH
fi

./simple_perf
