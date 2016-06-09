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
                       unzip                   \
                       curl                    \
                       git                     \
                       python-pip              \
                       python-dev              \
                       build-essential         \
                       libyaml-dev             \
                       mingw-w64               \
                       python-numpy

# NodeJS for javascript API
RUN apt-get -y install nodejs-legacy           \
                       npm                     \
                       nodejs                  \
                       default-jre
RUN npm install -g gulp
RUN npm install -g nodeunit
# Upgrade to Node 5 for Google closure compiler
RUN curl -sL https://deb.nodesource.com/setup_5.x | bash -
RUN apt-get install -y nodejs

RUN pip install --upgrade pip

WORKDIR $work_dir/
COPY . $work_dir/

RUN pip install -r $work_dir/requirements.txt
RUN bash $work_dir/install_eigen.sh
