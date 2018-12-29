#!/bin/sh

# First, from home folder do...
# note: but first on mac ONLY... ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)" ... then brew install wget --with-libressl ... then rename the download file to boost_1_65_0.tar.gz
# wget "https://sourceforge.net/projects/boost/files/boost/1.65.0/boost_1_65_0.tar.gz/download"
# then... tar -xvzf boost_1_65_0.tar.gz

cd ~/boost_1_65_0
export BOOST_HAS_THREADS ON
export BOOST_HAS_PTHREADS ON
export Boost_USE_MULTITHREADED ON
./bootstrap.sh --prefix=~/boostfedora 
./b2 install --prefix=~/boostfedora --layout=tagged --threading=multi --without-mpi --build-type=complete
cd ~
