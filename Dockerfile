# syntax=docker/dockerfile:1
FROM rootproject/root:6.26.10-ubuntu22.04
# FROM ubuntu:22.04
WORKDIR /playground
RUN sed -i 's|http://archive.ubuntu.com/ubuntu/|http://mirror.us-ny2.kamatera.com/ubuntu/|g' /etc/apt/sources.list && apt-get update && apt-get install -y python3 python3-numpy python3-h5py libhdf5-cpp-103 && apt-get clean && rm -rf /var/cache/apt/archives /var/lib/apt/lists
COPY --from=hic-eventgen-dev /root/.local/share/osu-hydro/ /usr/local/share/osu-hydro/
COPY --from=hic-eventgen-dev /bin/osu-hydro /bin/trento /bin/osc2u /bin/urqmd /bin/run-events-wrapper /bin/run-events /bin/afterburner /bin/hdf2root /bin/
COPY --from=hic-eventgen-dev /usr/local/lib/python3.10/ /usr/local/lib/python3.10/
COPY --from=hic-eventgen-dev /hic-eventgen/playground/ /playground/
COPY --from=hic-eventgen-dev /lib/x86_64-linux-gnu/libboost_program_options.so.1.74.0 /lib/x86_64-linux-gnu/libboost_program_options.so.1.74.0
COPY --from=hic-eventgen-dev /lib/x86_64-linux-gnu/libboost_filesystem.so.1.74.0 /lib/x86_64-linux-gnu/libboost_filesystem.so.1.74.0
COPY tools/help.txt /playground/help.txt
COPY tools/enc.cxx /playground/enc.cxx
CMD cat /playground/help.txt

