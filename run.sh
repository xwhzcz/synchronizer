#! /bin/bash

root_dir=$(cd `dirname $0`; pwd)
pid_file=$root_dir/sync.pid

function start() {
    cd $root_dir/build 
    ./my_sync > run.log &
    if [[ $? -eq 0 ]]; then
        echo $! > ${pid_file}
    else exit 1
    fi
}

function stop() {
    kill -9 $(cat ${pid_file})
    if [[ $? -eq 0 ]]; then
        rm -f ${pid_file}
    else exit 1
    fi
}

function call() {
    case $1 in
        'start')
            start
            ;;
        'stop')
            stop
            ;;
        'restart')
            stop
            start
            ;;
        *)
            echo 'Get invalid option, please input(as to $1):'
            echo -e '\t"start" -> start service'
            echo -e '\t"stop"  -> stop service'
            echo -e '\t"restart"  -> restart service'
                        exit 1
    esac
}

call "$1"