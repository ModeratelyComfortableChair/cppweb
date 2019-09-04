#Brings in gcc. best to specify version instead of use :latest
FROM gcc:7.3.0 

#-qq does it quietly (No output. Y to all prompts)
RUN apt-get -qq update
RUN apt-get -qq upgrade
RUN apt-get -qq install cmake

#Chrome microframework depends on boost
RUN apt-get -qq install libboost-all-dev=1.62.0.1
RUN apt-get -qq install build-essential libtcmalloc-minimal4 && \
  ln -s /usr/lib/libtcmalloc_minimal.so.4 /usr/lib/libtcmalloc_minimal.so