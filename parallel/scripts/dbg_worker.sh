script_dir=$(dirname "$0")

consome_arg(){
    arg=`echo $line | cut -d' ' -f 1`
    line=`echo $line | cut -d' ' -f 2- -s`
    
    if [[ "$arg" == "-w" ]]; then
        worker_config=`echo $line | cut -d' ' -f 1`
        line=`echo $line | cut -d' ' -f 2- -s`
    fi
}

line=`echo $@ | tr -s ' '`

echo "args: $line"

worker_config=./${script_dir}/../../configs/worker/worker_config_example.txt
manager_config=./${script_dir}/../../configs/manager/manager_config_example.txt

if [ -z $1 ]; then
    echo "No configuration file found... Using ${worker_config}"
    
elif [ "$1" == "-h" ] || [ "$1" == "--help" ]; then
    echo "usage: $0 [OPTIONS]"
    echo "OPTIONS:"
    echo "    -w <worker configuration file> "
    exit
else
    while [[ ! -z $line ]]
    do
        consome_arg
        echo $line
    done
fi


manager_bin=merge_manager
worker_bin=merge_worker

if [[ -f ${worker_config} ]]; then
   gdb --args ./${script_dir}/../exec/${worker_bin} ${worker_config}
else
    echo "File not found ${worker_config}"
fi
