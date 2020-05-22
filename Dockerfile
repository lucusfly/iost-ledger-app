# Run with shell command
# export APP_NAME=ledger-app-iost && docker build -t $APP_NAME . && docker run -it -p 4444:4444 -p 55555:55555 -v /dev:/dev --privileged --rm --name $APP_NAME  $APP_NAME 

FROM ubuntu:18.04
ENV ADPU_PORT 4444
ENV VNC_PORT 55555

RUN apt-get update
# common packages
RUN apt-get -y install \
    python3 wget
# arm cortex compiler
RUN apt-get -y install \
    llvm clang gcc-arm-none-eabi
#libstdc++-arm-none-eabi-newlib
# blue-loader-python dependencies
RUN apt-get -y install \
    libudev-dev libusb-1.0-0-dev python3-dev \
    protobuf-compiler python-protobuf
# speculos dpendencies
RUN apt-get -y install \
    python3-pil python3-pyelftools python3-mnemonic python3-setuptools python3-construct \
    cmake libvncserver-dev perl-modules qemu-user-static \
    gcc-arm-linux-gnueabihf gcc-multilib-arm-linux-gnueabi
# create limited user
RUN adduser --disabled-password --gecos "" bob
RUN usermod -aG nogroup bob
USER bob
WORKDIR /home/bob


# add sources and build sdk
ADD ./ ./.app
RUN cp -r .app app
RUN sed -i 's/-I\/usr\/include/-I\/usr\/arm-linux-gnueabi\/include/' app/sdk/nanos-secure-sdk/Makefile.defines
RUN cd app/sdk/python-yubicommon && \
    python3 setup.py install --user && \
    ln -s $(bash ../build-helpers/find-installed-package.sh yubicommon) ../python-u2flib-host/u2flib_host/yubicommon && \ 
    cd ../python-u2flib-host && \
    python3 setup.py install --user && \
    cd ../blue-loader-python && \
    python3 setup.py install --user

# build and install speculos
RUN cd app/sdk/speculos && \
    cmake -Bbuild -H. -DCMAKE_BUILD_TYPE=Debug -DWITH_VNC=1 && \
    make -C build -j3 && \
    make -C build install

# run app builder and emulator
RUN touch app/Makefile
EXPOSE ${ADPU_PORT}
EXPOSE ${VNC_PORT}
ENTRYPOINT ["python3", "app/x.py"]
