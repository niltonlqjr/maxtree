if [ -z $1 ]; then
    echo "script usage: ${0} <worker configuration file>"
    echo "configuration files examples in directory BASEDIR/maxtree/configs/worker"
    echo "this configuration files works when script is ran from BASEDIR/maxtree/parallel"
    exit 1
fi


worker_config=$1
worker_bin=merge_worker
script_dir=$(dirname "$0")

./${script_dir}/../exec/${worker_bin} ${worker_config}

