#!/bin/bash
set -e

branch=$(git rev-parse --abbrev-ref HEAD)

curl \
  --header "Content-Type: application/json" \
  --data '{"build_parameters": {"KOHERON_SERVER_BRANCH": "'$branch'"}}' \
  --request POST \
  https://circleci.com/api/v1.1/project/github/Koheron/koheron-python/tree/master?circle-token=$CIRCLE_TOKEN

curl \
  --header "Content-Type: application/json" \
  --data '{"build_parameters": {"KOHERON_SERVER_BRANCH": "'$branch'"}}' \
  --request POST \
  https://circleci.com/api/v1.1/project/github/Koheron/automation/tree/master?circle-token=$CIRCLE_TOKEN