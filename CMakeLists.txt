cmake_minimum_required(VERSION 3.7)
project(redis_api_server)

set(CMAKE_CXX_STANDARD 11)

add_compile_options(-O0)
add_compile_options(-ggdb3)
add_compile_options(-fpermissive)
add_compile_options(-Wall)
add_compile_options(-Wextra)
add_compile_options(-Wcast-align)
add_compile_options(-Wcast-qual)
add_compile_options(-Wconversion)
add_compile_options(-Weffc++)
add_compile_options(-Wformat=2)
add_compile_options(-Wmissing-format-attribute)
add_compile_options(-Wno-non-template-friend)
add_compile_options(-Wno-unused-parameter)
add_compile_options(-Woverloaded-virtual)
add_compile_options(-Wwrite-strings)
add_compile_options(-DSWIG)
add_compile_options(-fno-access-control)

set(THIRD_PARTY_DEPS ${CMAKE_CURRENT_SOURCE_DIR}/deps)
set(LOGCABIN_LIB ${CMAKE_CURRENT_SOURCE_DIR}/deps/logcabin/liblogcabin.a)

SET(SUBMODULE_LIBS ${CMAKE_CURRENT_SOURCE_DIR}/deps/third_party/gtest-c6d1811/libgtest.a
                   ${CMAKE_CURRENT_SOURCE_DIR}/deps/third_party/rocksdb-5.2.1/librocksdb.a)

if (APPLE)
    message(STATUS "Can not find library in non-Linux machine")
elseif (UNIX)
    find_library(LZ4_LIB_STATIC NAMES liblz4.a)
    find_library(PROTOBUF_LIB_STATIC NAMES "libprotobuf.a" PATHS /usr/lib)
    find_library(CRYPTOPP_LIB_STATIC NAMES libcrypto++.a)
    find_library(SNAPPY_LIB_STATIC NAMES libsnappy.a)
    find_library(BZ2_LIB_STATIC NAMES libbz2.a)
    find_library(JEMALLOC_LIB_STATIC NAMES libjemalloc.a)
    find_library(BOOST_SYSTEM_LIB_STATIC NAMES libboost_system.a)

    set(LIBS  ${SUBMODULE_LIBS}
            ${LZ4_LIB_STATIC}
            ${PROTOBUF_LIB_STATIC}
            ${CRYPTOPP_LIB_STATIC}
            pthread rt ${SNAPPY_LIB_STATIC}
            z ${BZ2_LIB_STATIC}
            ${JEMALLOC_LIB_STATIC}
            ${BOOST_SYSTEM_LIB_STATIC})
else ()
    message(STATUS "Can not find library in non-Linux machine")
endif ()

message(STATUS ${LIBS})
add_compile_options(-ggdb3)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}
                    ${THIRD_PARTY_DEPS}/ae
                    ${THIRD_PARTY_DEPS}/resp/resp
                    ${THIRD_PARTY_DEPS}/logcabin/include)

set(AE_SOURCE_FILES ${THIRD_PARTY_DEPS}/ae/ae_epoll.c
                    ${THIRD_PARTY_DEPS}/ae/ae.c
                    ${THIRD_PARTY_DEPS}/ae/anet.c)

set(SOURCE_FILES  redis_api_server.cpp  ${AE_SOURCE_FILES})
add_executable(redis_api_server ${SOURCE_FILES})
target_link_libraries(redis_api_server ${LOGCABIN_LIB} ${LIBS})