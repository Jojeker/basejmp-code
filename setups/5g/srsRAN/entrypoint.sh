#!/bin/bash

set -x

# Add additional echo statements to track progress
echo "Starting entrypoint script..."
echo "Running command with config: $1"

exec /usr/local/bin/gnb -c /gnb.yaml

# Execute the main command
#exec gnb $1
#cat /gnb.yaml
