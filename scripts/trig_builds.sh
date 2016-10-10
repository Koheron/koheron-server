#!/bin/bash
set -e

koheron_python_branch=master
automation_branch=master

branch=$(git rev-parse --abbrev-ref HEAD)

curl \
  --header "Content-Type: application/json" \
  --data '{"build_parameters": {"KOHERON_SERVER_BRANCH": "'$branch'"}}' \
  --request POST \
  https://circleci.com/api/v1.1/project/github/Koheron/koheron-python/tree/$koheron_python_branch?circle-token=$CIRCLE_TOKEN

curl \
  --header "Content-Type: application/json" \
  --data '{"build_parameters": {"KOHERON_SERVER_BRANCH": "'$branch'"}}' \
  --request POST \
  https://circleci.com/api/v1.1/project/github/Koheron/automation/tree/$automation_branch?circle-token=$CIRCLE_TOKEN
