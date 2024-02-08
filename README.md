# A Basic Real-time Chat Application

This project is a basic CLI-based real-time chat application written entirely in C.

## Prerequisites

Ensure you have Docker installed on your machine. If not, you can download and install it from [Docker's official website](https://www.docker.com/products/docker-desktop).

## Getting Started

Follow these steps to get the application up and running:

### 1. Build the Docker image

Run the build script to create the Docker image:

```bash
./build.sh
```
### 2. Run Server Instances
You can start one or more server instances by running the `server.sh` script with the number of instances as an argument:
```bash
./server.sh <# of instances>
```
Replace <# of instances> with the number of server instances you want to start.
### 3. Run Server Instances
Similarly, you can start one or more client instances by running the `client.sh` script with the number of instances as an argument:
```bash
./client.sh <# of instances>
```
In this README file, each step is clearly explained with the corresponding bash command provided in a code block for clarity. The placeholders `<# of instances>` are used to indicate where the user should specify the number of instances they want to start for the server and client.