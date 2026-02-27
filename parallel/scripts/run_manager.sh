if [ -z $1 ]; then
    echo "script usage: ${0} <manager configuration file>"
    echo "configuration files examples in directories BASEDIR/maxtree/configs/manager"
    echo "this configuration files works when script is ran from BASEDIR/maxtree/parallel"
    exit 1
fi

manager_config=$1
manager_bin=merge_manager
script_dir=$(dirname "$0")

./${script_dir}/../exec/${manager_bin} ${manager_config}


