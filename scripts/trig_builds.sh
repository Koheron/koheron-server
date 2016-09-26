#!/bin/bash
set -e

branch=$(git rev-parse --abbrev-ref HEAD)

for repo in koheron-python automation; do
    curl \
      --header "Content-Type: application/json" \
      --data '{"build_parameters": {"KOHERON_SERVER_BRANCH": "'$branch'"}}' \
      --request POST \
      https://circleci.com/api/v1.1/project/github/Koheron/$repo/tree/master?circle-token=$CIRCLE_TOKEN
done