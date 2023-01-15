.PHONY: install

hdf2root: hdf2root.cpp
	g++ -O3  -o hdf2root -IHighFive/include  -I/usr/include/hdf5/serial -L/usr/lib/x86_64-linux-gnu/hdf5/serial  hdf2root.cpp -lhdf5  `root-config --cflags --glibs`

install: hdf2root
	install -m 755 hdf2root /bin