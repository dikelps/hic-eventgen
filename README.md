# hic-eventgen-docker

This is a fork of the [original](https://github.com/Duke-QCD/hic-eventgen) to develop and run in containers. 

## Some features
- Docker support
- An example of how to convert the output of `h5` files to `root` files using [HighFive](https://github.com/BlueBrain/HighFive).
- Custom nucleus parameters for trento model (specify radius, skin-depth, etc.)

## Get started

    git clone --recursive https://github.com/dikelps/hic-eventgen
    cd hic-eventgen
    docker build --platform linux/arm64 -t hic-eventgen-dev -f dev.Dockerfile .
    docker run -it hic-eventgen-dev

Inside the container, you can play around and see if things work. If they seem to work, build the production image with the following.

    docker build --platform linux/arm64 -t hic-eventgen .

Usually latest commit will be built and pushed to the [docker hub](https://hub.docker.com/r/zhengxiyan/hic-eventgen).
