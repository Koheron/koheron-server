FROM ubuntu:14.04
#FROM armbuild/ubuntu:latest

RUN rm /bin/sh && ln -s /bin/bash /bin/sh

# install dependencies
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

WORKDIR /code/
COPY . /code/

# Compile kserverd
RUN make TARGET_HOST=local clean all
RUN make CROSS_COMPILE=arm-linux-gnueabihf- clean all
RUN make CROSS_COMPILE=arm-linux-gnueabi- clean all

# Compile CLI
RUN make -C cli TARGET_HOST=local clean all
RUN make -C cli CROSS_COMPILE=arm-linux-gnueabihf- clean all
RUN make -C cli CROSS_COMPILE=arm-linux-gnueabi- clean all
