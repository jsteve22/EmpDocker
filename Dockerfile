FROM debian:testing
# ARG AARCH=arm32v7
# FROM ${AARCH}/ubuntu

# RUN apt-get update && apt-get build-essential cmake --no-install-recommends
RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install \
    -y --no-install-recommends --no-install-suggests wget vim git sudo \ 
    libboost-all-dev cmake ca-certificates make g++ software-properties-common \
    build-essential libssl-dev
RUN apt-get install -y python3

COPY emp-sh2pc/ emp-sh2pc/

WORKDIR /emp-sh2pc/emp-tool
RUN cmake . && make -j4 && make install

WORKDIR /emp-sh2pc/emp-ot
RUN cmake . && make -j4 && make install

WORKDIR /emp-sh2pc
RUN cmake . && make -j4 && make install

WORKDIR /emp-sh2pc/bin
