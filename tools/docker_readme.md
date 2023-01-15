# docker-hic-eventgen

This image targets singularity/apptainer users. \
Download and build image in current directory with `pull`.
```
singularty pull docker://zhengxiyan/hic-eventgen
```
You might see some warnings, but as it says they are harmless. After a `.sif` image is generated, run the image with `run` for more details.
```
singulartiy run hic-eventgen_latest.sif
```