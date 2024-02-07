#!/bin/bash
if [ "$1" = 'server' ]; then
    shift
    ./chat-c/srvr "$@"
elif [ "$1" = 'client' ]; then
    shift
    ./chat-c/client_s "$@"
else
    echo "Please specify 'server' or 'client'"
    exit 1
fi