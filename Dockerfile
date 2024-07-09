FROM ubuntu:latest
ENV DEBIAN_FRONTEND=noninteractive

# Install necessary tools and libraries
RUN apt-get update && apt-get install -y \
    build-essential \
    wget \
    unzip \
    libsqlite3-dev \
    libboost-all-dev

RUN wget https://botan.randombit.net/releases/Botan-3.4.0.tgz && \
    tar xzf Botan-3.4.0.tgz && \
    cd Botan-3.4.0 && \
    ./configure.py --prefix=/usr/local && \
    make && \
    make install && \
    cd .. && \
    rm -rf Botan-3.4.0.tgz Botan-3.4.0

# Set the working directory in the container
WORKDIR /app
COPY . .

# Compile and run the project using g++
RUN g++ -std=c++20 -o denim src/main.cpp -I/usr/local/include/botan-3 -I/usr/include/sqlite3 -lboost_system -lboost_filesystem -lboost_serialization -lsqlite3 -lbotan-3
CMD ["./denim"]
