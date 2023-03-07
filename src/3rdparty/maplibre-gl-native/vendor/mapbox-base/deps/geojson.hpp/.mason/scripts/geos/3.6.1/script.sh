#!/usr/bin/env bash

MASON_NAME=geos
MASON_VERSION=3.6.1
MASON_LIB_FILE=lib/libgeos.a

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
        https://download.osgeo.org/geos/${MASON_NAME}-${MASON_VERSION}.tar.bz2 \
        c23fa59e6ab8a4e8df634773fb1ac9794fc5d88a

    mason_extract_tar_bz2

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/${MASON_NAME}-${MASON_VERSION}
}

function mason_compile {
    #if [ "$MASON_PLATFORM" == "linux" ]; then
        mason_step "Loading patch ${MASON_DIR}/scripts/${MASON_NAME}/${MASON_VERSION}/patch.diff"
        patch -N -p1 < ${MASON_DIR}/scripts/${MASON_NAME}/${MASON_VERSION}/patch.diff
    #fi

    # note: we put ${STDLIB_CXXFLAGS} into CXX instead of LDFLAGS due to libtool oddity:
    # https://stackoverflow.com/questions/16248360/autotools-libtool-link-library-with-libstdc-despite-stdlib-libc-option-pass
    if [[ $(uname -s) == 'Darwin' ]]; then
        CXX="${CXX} -stdlib=libc++ -std=c++11"
    fi
    export CFLAGS="${CFLAGS} -O3 -DNDEBUG"
    export CXXFLAGS="${CXXFLAGS} -O3 -DNDEBUG"
    ./configure \
        --prefix=${MASON_PREFIX} \
        ${MASON_HOST_ARG} \
        --disable-shared --enable-static \
        --disable-dependency-tracking
    make -j${MASON_CONCURRENCY} install
}

function mason_cflags {
    echo $(${MASON_PREFIX}/bin/geos-config --cflags)
}

function mason_ldflags {
    echo $(${MASON_PREFIX}/bin/geos-config  --static-clibs)
}

function mason_clean {
    make clean
}

mason_run "$@"
