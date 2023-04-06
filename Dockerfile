FROM debian:testing
# ARG AARCH=arm32v7
# FROM ${AARCH}/ubuntu

# RUN apt-get update && apt-get build-essential cmake --no-install-recommends
RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install \
    -y --no-install-recommends --no-install-suggests wget vim git sudo \ 
    libboost-all-dev cmake ca-certificates make g++
RUN apt-get install -y python3

RUN git clone git@github.com:emp-toolkit/emp-sh2pc.git

COPY emp-sh2pc/ emp-sh2pc/

WORKDIR /emp-sh2pc
RUN wget https://raw.githubusercontent.com/emp-toolkit/emp-readme/master/scripts/install.py && python3 install.py --deps --tool --ot --sh2pc

RUN cmake . && make -j4 && make install

WORKDIR bin
