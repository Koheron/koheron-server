FROM ubuntu:14.04
#FROM armbuild/ubuntu:latest

ENV work_dir /code

# ---------------------------------------
# Install dependencies
# ---------------------------------------

RUN apt-get -y install software-properties-common
RUN add-apt-repository -y ppa:ubuntu-toolchain-r/test
RUN apt-get update

RUN apt-get -y install gcc-4.8 g++-4.8
RUN update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-4.8 60 \
                        --slave /usr/bin/g++ g++ /usr/bin/g++-4.8
                        
RUN apt-get -y install gcc-arm-linux-gnueabihf
RUN apt-get -y install g++-arm-linux-gnueabihf
RUN apt-get -y install gcc-arm-linux-gnueabi
RUN apt-get -y install g++-arm-linux-gnueabi

RUN apt-get -y install make
RUN apt-get -y install wget

# Install virtualenv
RUN apt-get -y install python-pip python-dev build-essential python-virtualenv
RUN pip install --upgrade pip

WORKDIR $work_dir/
COPY . $work_dir/

RUN pip install -r requirements.txt

# ---------------------------------------
# Compile kserverd
# ---------------------------------------

RUN make DOCKER=True CONFIG=config_local.yaml clean all
RUN make DOCKER=True CONFIG=config_armel.yaml clean all
RUN make DOCKER=True CONFIG=config_armhf.yaml clean all
RUN make DOCKER=True CONFIG=config_toolchain.yaml clean all

# ---------------------------------------
# Compile CLI
# ---------------------------------------

RUN make -C cli TARGET_HOST=local clean all
RUN make -C cli CROSS_COMPILE=arm-linux-gnueabihf- clean all
RUN make -C cli CROSS_COMPILE=arm-linux-gnueabi- clean all

# ---------------------------------------
# Tests
# ---------------------------------------

# Compile server in local
RUN make DOCKER=True CONFIG=config_local.yaml clean all
RUN make -C cli TARGET_HOST=local clean all

# Launch kserver
RUN $work_dir/tmp/server/kserverd -c $work_dir/config/kserver_docker.conf &
#RUN ps -A | grep kserverd
RUN cat /var/log/syslog | grep --text KServer

RUN $work_dir/cli/kserver host --tcp localhost 36000
RUN $work_dir/cli/kserver host --status
RUN $work_dir/cli/kserver status --sessions
