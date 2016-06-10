#!/bin/bash

VERSION=2e37c304442e

wget -P /tmp https://bitbucket.org/eigen/eigen/get/${VERSION}.zip
unzip -o /tmp/${VERSION}.zip -d /tmp
cp -r /tmp/eigen-eigen-${VERSION}/Eigen /usr/include