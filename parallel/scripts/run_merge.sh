script_dir=$(dirname "$0")
if [[ "$1" == "-h" ]] || [[ "$1" == "--help" ]]; then
    echo "usage $0 [manager config file] [worker config file]"
    exit
elif [ -z $1 ] || [ -f ${manager_config} ]; then
    manager_config=./${script_dir}/../../configs/manager/manager_config_example.txt
    echo "No configuration manager file found... Using ${manager_config}"
else
    manager_config=$1
fi

if [ -z $2 ] || [ -f ${worker_config} ]; then
    worker_config=./${script_dir}/../../configs/worker/worker_config_example.txt
    echo "No configuration file found... Using ${worker_config}"
else
    worker_config=$2
fi

manager_bin=merge_manager
worker_bin=merge_worker


./${script_dir}/../exec/${manager_bin} ${manager_config} &
./${script_dir}/../exec/${worker_bin} ${worker_config}

