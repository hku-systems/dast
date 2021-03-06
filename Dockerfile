FROM ubuntu:16.04


# setup ssh server
RUN apt-get update && apt-get install openssh-server -y

RUN mkdir /var/run/sshd

# SSH login fix. Otherwise user is kicked off after login
RUN sed 's@session\s*required\s*pam_loginuid.so@session optional pam_loginuid.so@g' -i /etc/pam.d/sshd

RUN mkdir -p ~/.ssh && ssh-keygen -t rsa -N '' -f ~/.ssh/id_rsa && cat ~/.ssh/id_rsa.pub >> ~/.ssh/authorized_keys

RUN ssh-keyscan -H 127.0.0.1 >> ~/.ssh/known_hosts

COPY ./requirements.txt /root/

WORKDIR /root

RUN apt-get update && apt-get install -y \
    git \
    pkg-config \
    build-essential \
    clang \
    libapr1-dev libaprutil1-dev \
    libboost-all-dev \
    libyaml-cpp-dev \
    python-dev \
    python-pip \
    python-tk\
    libgoogle-perftools-dev \
    net-tools \
    iputils-ping \
    psmisc

RUN pip install --upgrade "pip < 21.0"
RUN pip install -r /root/requirements.txt
