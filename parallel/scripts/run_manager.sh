script_dir=$(dirname "$0")
if [[ "$1" == "-h" ]] || [[ "$1" == "--help" ]]; then
    echo "usage $0 [manager config file] [worker config file]"
    exit
elif [ -z $1 ] || [ -f ${manager_config} ]; then
    manager_config=./${script_dir}/../../configs/manager/manager_config_example.txt
    echo "No configuration file found... Using ${manager_config}"
else
    manager_config=$1
fi
manager_bin=merge_manager


./${script_dir}/../exec/${manager_bin} ${manager_config}


