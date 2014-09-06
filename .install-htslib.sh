#!/bin/bash
set -x
DIR=${TRAVIS_BUILD_DIR}
git clone https://github.com/samtools/htslib.git ${DIR}/htslib
cd ${DIR}/htslib
git checkout develop

make

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$(pwd)
export C_INCLUDE_PATH=$(pwd):${DIR}

sudo ldconfig
