#!/bin/bash
apt-get update
apt-get -y install npm nodejs-legacy
npm install -g grunt grunt-cli bower
npm install
bower --allow-root install
grunt build
