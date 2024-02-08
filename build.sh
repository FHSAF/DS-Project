#!/bin/bash

IMAGE_NAME="chat-c"
DOCKERFILE_PATH="."
NETWORK_NAME="ds_network"
SUBNET="192.168.0.0/24"

# Build the Docker image
docker build -t $IMAGE_NAME $DOCKERFILE_PATH

# Check if the network already exists
if [ "$(docker network ls -q -f name=$NETWORK_NAME)" ]; then
    # If it does, remove it
    docker network rm $NETWORK_NAME
fi

# Create the network
docker network create --subnet=$SUBNET $NETWORK_NAME