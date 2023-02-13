# syntax=docker/dockerfile:1
FROM rootproject/root:6.26.10-ubuntu22.04
WORKDIR /hic-eventgen
ENV DEBIAN_FRONTEND noninteractive
ENV DEBCONF_NOWARNINGS="yes"
RUN sed -i 's|http://archive.ubuntu.com/ubuntu/|http://mirror.us-ny2.kamatera.com/ubuntu/|g' /etc/apt/sources.list
RUN apt-get update
RUN apt-get install -y libhdf5-dev python3-h5py libboost-all-dev cmake python3-setuptools cython3 gfortran build-essential
RUN apt-get install -y vim

COPY tools/pre_install.sh  tools/
run bash tools/pre_install.sh

COPY tools/install_model.sh tools/

COPY models/urqmd-afterburner models/urqmd-afterburner
RUN bash tools/install_model.sh models/urqmd-afterburner

COPY models/freestream models/freestream
RUN bash tools/install_model.sh models/freestream

COPY models/frzout models/frzout
RUN bash tools/install_model.sh models/frzout

COPY models/osu-hydro models/osu-hydro
RUN bash tools/install_model.sh models/osu-hydro


COPY tools/install_run_event.sh tools/
COPY models/run-events-wrapper models/run-events-wrapper
COPY models/run-events models/run-events
RUN bash tools/install_run_event.sh

COPY models/trento models/trento
RUN bash tools/install_model.sh models/trento

COPY tools/hdf2root tools/hdf2root
RUN bash tools/hdf2root/install.sh tools/hdf2root 

COPY tools/post_install.sh tools/
RUN bash tools/post_install.sh

COPY tools/test_pars playground/
COPY tools/test.sh playground/
COPY tools/enc.cxx playground/
# RUN bash ./multi_stage_build_script/build_hdf2root.sh

CMD bash

