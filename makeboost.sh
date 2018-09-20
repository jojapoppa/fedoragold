#!/bin/sh

# First, from home folder do...
# wget "https://sourceforge.net/projects/boost/files/boost/1.65.0/boost_1_65_0.tar.gz/download"
# then... tar -xvzf boost_1_65_0.tar.gz

cd ~/boost_1_65_0
export BOOST_HAS_THREADS ON
export BOOST_HAS_PTHREADS ON
export Boost_USE_MULTITHREADED ON
./bootstrap.sh --prefix=/home/fork/boostfedora 
./b2 install --prefix=/home/fork/boostfedora --layout=tagged --threading=multi --without-mpi --build-type=complete
cd ~
