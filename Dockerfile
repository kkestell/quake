FROM i386/debian:stable

RUN apt-get update && \
    apt-get install -y \
        build-essential \
        cmake \
        libsdl2-dev \
        libsdl2-mixer-dev \
        libstdc++6 \
        libx11-6 \
        libxext6 \
        libxrandr2 \
        libxrender1 \
        libxfixes3

WORKDIR /quake
COPY . /quake

RUN cmake . && make
RUN mkdir -p /quake/id1
COPY id1 /quake/id1

CMD ["./quake"]
