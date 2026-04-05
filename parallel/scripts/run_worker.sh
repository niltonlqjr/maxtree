worker_bin=merge_worker
script_dir=$(dirname "$0")
if [[ $1 == "-h" ]] || [[ $1 == "--help" ]]; then
    echo "usage $0 [worker config file] [number of merge_worker process]"
    echo "each merge_worker can have multiple threads"
    exit
elif [[ -z $1 ]] || ! [[ -f ${worker_config} ]]; then
    worker_config=./${script_dir}/../../configs/worker/worker_config_example.txt
    echo "No configuration file found... Using ${worker_config}"
else
    worker_config=$1
    
fi

if ! [[ -z $2 ]] || [[ $2 =~ ^[0-9]+$ ]]; then
    num_process=$2
else
    num_process=1
fi

#worker_cmd=./${script_dir}/../exec/${worker_bin} ${worker_config}

if [[ ${num_process} -gt "1" ]]; then
    for _c in `seq ${num_process}`; do
        ./${script_dir}/../exec/${worker_bin} ${worker_config} &
        #`${worker_cmd} &`
        
    done
else
    ./${script_dir}/../exec/${worker_bin} ${worker_config}
    #`${worker_cmd}`
fi
