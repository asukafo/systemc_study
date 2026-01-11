export SYSTEMC_HOME=/opt/systemc-2.3.3
export LD_LIBRARY_PATH=$SYSTEMC_HOME/lib:$LD_LIBRARY_PATH

#args=("$@")        
#first="${args[0]}"  
#second="${args[1]}" 

#./perf_model first

./perf_model
