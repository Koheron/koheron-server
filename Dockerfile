FROM ubuntu:16.04
#FROM armbuild/ubuntu:latest

ENV work_dir /code

# ---------------------------------------
# Install dependencies
# ---------------------------------------

RUN apt-get update && apt-get -y install gcc-5 g++-5             \
                       gcc-arm-linux-gnueabihf \
                       g++-arm-linux-gnueabihf \
                       gcc-arm-linux-gnueabi   \
                       g++-arm-linux-gnueabi   \
                       make                    \
                       wget                    \
                       unzip                   \
                       zip                     \
                       curl                    \
                       git                     \
                       python-pip              \
                       python-dev              \
                       build-essential         \
                       libyaml-dev             \
                       mingw-w64               \
                       python-numpy	       \
                       python3-numpy	       \
                       python3-pip

# NodeJS for javascript API
RUN apt-get -y install nodejs-legacy           \
                       npm                     \
                       nodejs                  \
                       default-jre

# Upgrade to Node 6 for Google closure compiler
RUN curl -sL https://deb.nodesource.com/setup_6.x | bash -
RUN apt-get install -y nodejs

# Upgrade npm to latest version
RUN npm update npm -g

# https://github.com/OpenCollective/opencollective-api/issues/311
RUN npm --version
RUN node --version

RUN npm install -g gulp
RUN npm install -g nodeunit

RUN pip install --upgrade pip

WORKDIR $work_dir/
COPY . $work_dir/

RUN pip install -r $work_dir/requirements.txt
RUN pip install wsgiref==0.1.2
RUN pip install -r $work_dir/tests/requirements.txt
RUN pip3 install -r $work_dir/tests/requirements.txt
RUN curl https://raw.githubusercontent.com/Koheron/koheron-python/master/install.sh | sudo /bin/bash /dev/stdin python cli
RUN curl https://raw.githubusercontent.com/Koheron/koheron-python/master/install.sh | sudo /bin/bash /dev/stdin python3 cli
RUN bash $work_dir/scripts/install_eigen.sh
