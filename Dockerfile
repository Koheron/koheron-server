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
                       curl                    \
                       git                     \
                       python-pip              \
                       python-dev              \
                       build-essential         \
                       libyaml-dev             \
                       mingw-w64               \
                       python-numpy

# NodeJS for javascript API
RUN apt-get -y install npm                     \
                       node                    \
                       nodejs-legacy           \
                       default-jre
RUN npm install -g gulp
# Upgrade to Node 5 for Google closure compiler
RUN curl -sL https://deb.nodesource.com/setup_5.x | sudo -E bash -
RUN apt-get install -y nodejs

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
# Build js API
# ---------------------------------------
RUN make -C APIs/js/koheron-websocket-client build

# ---------------------------------------
# Tests
# ---------------------------------------

# Compile executables in local for tests
RUN make BUILD_LOCAL=True DOCKER=True CONFIG=config_local.yaml clean all
RUN make -C cli TARGET_HOST=local clean all
RUN make -C APIs/C/tests TARGET_HOST=local clean all
