#!/bin/bash

docker rm $(sudo docker stop $(sudo docker ps -a -q --filter ancestor=10.22.1.1:5000/dast --format="{{.ID}}"))