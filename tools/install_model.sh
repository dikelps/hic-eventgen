pushd $1

if [[ -f CMakeLists.txt ]]; then
    # create build directory and run cmake if necessary

    rm -rf build
    mkdir build && cd build
    # subshell allows temporary environment modification
    (
        [[ $i == *urqmd-afterburner* && $NERSC_HOST ]] && {
            # urqmd has a fatal runtime error when compiled by Intel
            module swap PrgEnv-{intel,gnu}
            export FC=gfortran FFLAGS=$GNU_FLAGS
        }
        echo here
        exec cmake \
            -DCMAKE_BUILD_TYPE=Release \
            -DCMAKE_INSTALL_PREFIX=/ \
            ..
    ) || exit 1

    # compile and install
    make --jobs=$(nproc) DESTDIR=$prefix install || exit 1
elif [[ -f setup.py ]]; then
    # install python packages
    # subshell allows temporary environment modification
    (
        [[ $PY_FLAGS ]] && export CFLAGS=$PY_FLAGS CXXFLAGS=$PY_FLAGS
        exec python3.10 setup.py install
    ) || exit 1
else
    echo "unknown build system for model $i"
    exit 1
fi
popd
