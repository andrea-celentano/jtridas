project(TrigJANA CXX)
cmake_minimum_required(VERSION 3.5)


if (CMAKE_COMPILER_IS_GNUCXX)
    message(STATUS "Using GNU c++ compiler")
    set(CMAKE_CXX_FLAGS "-pthread -pipe -Wall -Wextra -Wno-unused-local-typedefs -Wno-unused-parameter -std=c++11")
    set(CMAKE_CXX_FLAGS_RELEASE "-O2")
    set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g3")
elseif (CMAKE_CXX_COMPILER_ID MATCHES Clang)
    message(STATUS "Using clang compiler")
    set(CMAKE_CXX_FLAGS "-pipe -Wall -Wextra")
    set(CMAKE_CXX_FLAGS_RELEASE "-O2")
    set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g3")
else (CMAKE_COMPILER_IS_GNUCXX)
    message(FATAL_ERROR "${CMAKE_CXX_COMPILER} is not a supported compiler")
endif (CMAKE_COMPILER_IS_GNUCXX)

if (NOT DEFINED ENV{TRIDAS_HOME})
	message (FATAL_ERROR "Variable TRIDAS_HOME not set")
else (NOT DEFINED ENV{TRIDAS_HOME})
	message (STATUS "TRIDAS_HOME set to:  $ENV{TRIDAS_HOME}")
endif (NOT DEFINED ENV{TRIDAS_HOME})
set (TRIDAS_SOURCEDIR $ENV{TRIDAS_HOME})

if (NOT DEFINED ENV{JANA_HOME})
	message (FATAL_ERROR "Variable JANA_HOME not set")
else (NOT DEFINED ENV{JANA_HOME})
	message (STATUS "JANA_HOME set to:  $ENV{JANA_HOME}")
endif (NOT DEFINED ENV{JANA_HOME})
set (JANA_SOURCEDIR $ENV{JANA_HOME}/src/lib/)
set (JANA_LIBDIR $ENV{JANA_HOME}/lib)

if (NOT DEFINED ENV{CCDB_HOME})
	message (FATAL_ERROR "Variable CCDB_HOME not set")
else (NOT DEFINED ENV{CCDB_HOME})
	message (STATUS "CCDB_HOME set to:  $ENV{CCDB_HOME}")
endif (NOT DEFINED ENV{CCDB_HOME})
set (CCDB_INCLUDE_DIR $ENV{CCDB_HOME}/include/)
set (CCDB_LIBDIR $ENV{CCDB_HOME}/lib)


if (NOT DEFINED ENV{RECO_HOME})
	message (FATAL_ERROR "Variable RECO_HOME not set")
else (NOT DEFINED ENV{RECO_HOME})
	message (STATUS "RECO_HOME set to:  $ENV{RECO_HOME}")
endif (NOT DEFINED ENV{RECO_HOME})
set (RECO_SOURCEDIR $ENV{RECO_HOME}/src)
set (RECO_LIBDIR $ENV{RECO_HOME}/lib)

if (NOT DEFINED ENV{ROOTSYS})
        message (FATAL_ERROR "Variable ROOTSYS not set")
else (NOT DEFINED ENV{ROOTSYS})
        message (STATUS "ROOTSYS set to:  $ENV{ROOTSYS}")
endif (NOT DEFINED ENV{ROOTSYS})
set (ROOT_INCLUDE_DIR $ENV{ROOTSYS}/include)

# The following was part of a merge conflict that I don't know how to handle.  2020-02-14 DL
#set (ROOT_LIBDIR $ENV{ROOTSYS}/lib)


list(APPEND CMAKE_PREFIX_PATH $ENV{ROOTSYS}) 
find_package(ROOT REQUIRED COMPONENTS Proof) 

set(CXX_STANDARD_REQUIRED "On")
set_property(GLOBAL PROPERTY CXX_STANDARD 11)



find_package(Boost 1.66 EXACT REQUIRED COMPONENTS thread system random program_options regex filesystem)
#find_package(ZMQ 4.0.4 REQUIRED)



set(
  PACKAGES_INCLUDE_DIRS
  ${TRIDAS_SOURCEDIR}/packages/Buffer 
  ${TRIDAS_SOURCEDIR}/packages/Configuration 
  ${TRIDAS_SOURCEDIR}/packages/DataFormats 
  ${TRIDAS_SOURCEDIR}/packages/Log 
  ${TRIDAS_SOURCEDIR}/packages/network 
  ${TRIDAS_SOURCEDIR}/packages/monitoring 
  ${TRIDAS_SOURCEDIR}/packages/Queue 
  ${TRIDAS_SOURCEDIR}/packages/Socket 
  ${TRIDAS_SOURCEDIR}/packages/TimeSlice 
  ${TRIDAS_SOURCEDIR}/packages/DataFormats 
  ${TRIDAS_SOURCEDIR}/packages/data_consistency 
  ${TRIDAS_SOURCEDIR}/packages/datacard 
  ${TRIDAS_SOURCEDIR}/packages/utility 
  ${TRIDAS_SOURCEDIR}/DAQ/TCPU/inc
  ${JANA_SOURCEDIR}
  ${RECO_SOURCEDIR}/libraries
  ${Boost_INCLUDE_DIR}
  ${CCDB_INCLUDE_DIR}
  ${ROOT_INCLUDE_DIR}
  CACHE INTERNAL ""
)
   
include_directories(
API 
${PACKAGES_INCLUDE_DIRS}
)

link_directories(
 ${JANA_LIBDIR}
 ${RECO_LIBDIR}
 ${CCDB_LIBDIR}
 ${ROOT_LIBDIR}
)

add_library(TrigJANA MODULE
    API/JEventSource_Tridas.cc
    API/GroupedEventProcessor.cc
    TrigJANA.cpp
    )

target_link_libraries(
  TrigJANA
  ${Boost_LIBRARIES}
  JANA
  streamingReco
  ccdb
)

