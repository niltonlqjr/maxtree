if [ -z $1 ] || [ -z $2 ]; then
    echo "script usage: ${0} <manager configuration file> <worker configuration file>"
    echo "configuration files examples in directories BASEDIR/maxtree/configs/manager and BASEDIR/maxtree/configs/worker"
    echo "this configuration files works when script is ran from BASEDIR/maxtree/parallel"
    exit 1
fi

manager_config=$1
worker_config=$2
manager_bin=merge_manager
worker_bin=merge_worker
script_dir=$(dirname "$0")

./${script_dir}/../exec/${manager_bin} ${manager_config} &
./${script_dir}/../exec/${worker_bin} ${worker_config}

