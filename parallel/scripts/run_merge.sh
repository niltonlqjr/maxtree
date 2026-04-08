script_dir=$(dirname "$0")

consome_arg(){
    arg=`echo $line | cut -d' ' -f 1`
    line=`echo $line | cut -d' ' -f 2- -s`
    
    if [[ "$arg" == "-w" ]]; then
        worker_config=`echo $line | cut -d' ' -f 1`
        line=`echo $line | cut -d' ' -f 2- -s`
    elif [[ "$arg" == "-m" ]]; then
        manager_config=`echo $line | cut -d' ' -f 1`
        line=`echo $line | cut -d' ' -f 2- -s`
    fi
}

line=`echo $@ | tr -s ' '`

echo $line

worker_config=./${script_dir}/../../configs/worker/worker_config_example.txt
manager_config=./${script_dir}/../../configs/manager/manager_config_example.txt

if [ -z $1 ]; then
        echo "No configuration file found... Using ${worker_config}"
        echo "No configuration manager file found... Using ${manager_config}"
else
    while [[ ! -z $line ]]
    do
        consome_arg
        echo $line
    done
fi


manager_bin=merge_manager
worker_bin=merge_worker

if [[ -f ${manager_config} ]] && [[ -f ${worker_config} ]]; then
    ./${script_dir}/../exec/${manager_bin} ${manager_config} &
    ./${script_dir}/../exec/${worker_bin} ${worker_config}
elif [[ ! -f ${manager_config} ]]; then
    echo "File not found ${manager_config}"
else
    echo "File not found ${worker_config}"
fi
