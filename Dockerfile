FROM ubuntu:16.04
#FROM armbuild/ubuntu:latest

ENV work_dir /code

# ---------------------------------------
# Install dependencies
# ---------------------------------------

RUN apt-get update
RUN apt-get -y install gcc-5 g++-5             \
                       gcc-arm-linux-gnueabihf \
                       g++-arm-linux-gnueabihf \
                       gcc-arm-linux-gnueabi   \
                       g++-arm-linux-gnueabi   \
                       make                    \
                       wget                    \
                       git                     \
                       python-pip              \
                       python-dev              \
                       build-essential         \
                       libyaml-dev             \
                       mingw-w64

RUN pip install --upgrade pip

WORKDIR $work_dir/
COPY . $work_dir/

RUN pip install -r requirements.txt

# ---------------------------------------
# Compile kserverd
# ---------------------------------------

RUN make BUILD_LOCAL=True DOCKER=True CONFIG=config_local.yaml clean all
RUN make BUILD_LOCAL=True DOCKER=True CONFIG=config_armel.yaml clean all
RUN make BUILD_LOCAL=True DOCKER=True CONFIG=config_armhf.yaml clean all
RUN make BUILD_LOCAL=True DOCKER=True CONFIG=config_toolchain.yaml clean all

# ---------------------------------------
# Compile CLI
# ---------------------------------------

RUN make -C cli TARGET_HOST=local clean all
RUN make -C cli CROSS_COMPILE=arm-linux-gnueabihf- clean all
RUN make -C cli CROSS_COMPILE=arm-linux-gnueabi- clean all

# ---------------------------------------
# Compile API Tests
# ---------------------------------------

RUN make -C APIs/C/tests TARGET_HOST=local clean all
RUN make -C APIs/C/tests TARGET_HOST=arm clean all
RUN make -C APIs/C/tests TARGET_HOST=armhf clean all
RUN make -C APIs/C/tests TARGET_HOST=Win32 clean all
RUN make -C APIs/C/tests TARGET_HOST=Win64 clean all

# ---------------------------------------
# Tests
# ---------------------------------------

# Compile server in local
RUN make BUILD_LOCAL=True DOCKER=True CONFIG=config_local.yaml clean all
RUN make -C cli TARGET_HOST=local clean all

# Launch kserver
RUN $work_dir/tmp/server/kserverd -c $work_dir/config/kserver_docker.conf &
EXPOSE 36000
#RUN ps -A | grep kserverd

#RUN $work_dir/cli/kserver host --tcp localhost 36000
#RUN $work_dir/cli/kserver host --status
#RUN $work_dir/cli/kserver status --sessions
