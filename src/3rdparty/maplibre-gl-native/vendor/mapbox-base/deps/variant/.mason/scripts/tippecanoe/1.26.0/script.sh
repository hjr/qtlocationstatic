#!/usr/bin/env bash

MASON_NAME=tippecanoe
MASON_VERSION=1.26.0
MASON_LIB_FILE=bin/tippecanoe

. ${MASON_DIR}/mason.sh

function mason_load_source {
    mason_download \
        https://github.com/mapbox/tippecanoe/archive/${MASON_VERSION}.tar.gz \
        1a2e27eada195d199a7df1d3e74aa7e281c51157

    mason_extract_tar_gz

    export MASON_BUILD_PATH=${MASON_ROOT}/.build/${MASON_NAME}-${MASON_VERSION}
}

SQLITE_VERSION=3.16.2

function mason_prepare_compile {
    ${MASON_DIR}/mason install sqlite ${SQLITE_VERSION}
    MASON_SQLITE=$(${MASON_DIR}/mason prefix sqlite ${SQLITE_VERSION})
}

function mason_compile {
    # knock out /usr/local to ensure libsqlite without a doubt that
    # sqlite from from mason is used
    perl -i -p -e "s/-L\/usr\/local\/lib//g;" Makefile
    perl -i -p -e "s/-I\/usr\/local\/include//g;" Makefile
    if [[ $(uname -s) == 'Darwin' ]]; then
        LDFLAGS="${LDFLAGS} -stdlib=libc++"
    fi

    PREFIX=${MASON_PREFIX} \
    PATH=${MASON_SQLITE}/bin:${PATH} \
    CXXFLAGS="${CXXFLAGS} -I${MASON_SQLITE}/include" \
    LDFLAGS="${LDFLAGS} -L${MASON_SQLITE}/lib -ldl -lpthread" make

    PREFIX=${MASON_PREFIX} \
    PATH=${MASON_SQLITE}/bin:${PATH} \
    CXXFLAGS="${CXXFLAGS} -I${MASON_SQLITE}/include" \
    LDFLAGS="${LDFLAGS} -L${MASON_SQLITE}/lib -ldl -lpthread" make install
}

function mason_cflags {
    :
}

function mason_ldflags {
    :
}

function mason_static_libs {
    :
}

function mason_clean {
    make clean
}

mason_run "$@"
