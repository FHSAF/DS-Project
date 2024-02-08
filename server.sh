#!/bin/bash

if [ $# -eq 0 ]; then
	echo "Please provide an integer argument."
	exit 1
fi

ARGUMENT=$1

for ((i=0; i<=$ARGUMENT; i++))
do
	CONTAINER_NAME="S$i"
	if [ "$(docker ps -aq -f name=$CONTAINER_NAME)" ]; then
		# If it exists, stop and remove it
		docker stop $CONTAINER_NAME
		docker rm $CONTAINER_NAME
	fi
	sleep 5
	osascript -e "tell application \"Terminal\" to do script \"docker run -it --network=ds_network --ip=192.168.0.$((10+i)) --name=$CONTAINER_NAME chat-c server 192.168.0.$((10+i)) 5000\""
done
