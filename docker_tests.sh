# /bin/bash
tmp/server/kserverd -c config/kserver_docker.conf&
ps -A

cli/kserver host --tcp localhost 36000
cli/kserver host --status
cli/kserver status --sessions
