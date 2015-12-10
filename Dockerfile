FROM ubuntu:14.04
#FROM armbuild/ubuntu:latest

# Install dependencies
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

# Install virtualenv
RUN apt-get -y install python-pip python-dev build-essential python-virtualenv
RUN pip install --upgrade pip

WORKDIR /code/
COPY . /code/

RUN pip install -r requirements.txt

# Compile kserverd
RUN make DOCKER=True CONFIG=config_local.yaml clean all
RUN make DOCKER=True CONFIG=config_armel.yaml clean all
RUN make DOCKER=True CONFIG=config_armhf.yaml clean all

# Compile CLI
RUN make -C cli TARGET_HOST=local clean all
RUN make -C cli CROSS_COMPILE=arm-linux-gnueabihf- clean all
RUN make -C cli CROSS_COMPILE=arm-linux-gnueabi- clean all
