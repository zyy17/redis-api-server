cmake_minimum_required(VERSION 3.7)
project(redis_api_server)

set(CMAKE_CXX_STANDARD 11)
set(THIRD_PARTY_DEPS ${CMAKE_CURRENT_SOURCE_DIR}/deps)

add_compile_options(-ggdb3)

include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${THIRD_PARTY_DEPS}/ae ${THIRD_PARTY_DEPS}/resp/resp)

set(AE_SOURCE_FILES ${THIRD_PARTY_DEPS}/ae/ae_epoll.c
                    ${THIRD_PARTY_DEPS}/ae/ae.c
                    ${THIRD_PARTY_DEPS}/ae/anet.c)

set(SOURCE_FILES  redis_api_server.cpp  ${AE_SOURCE_FILES})
add_executable(redis_api_server ${SOURCE_FILES})