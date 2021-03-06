PROJECT(libswoole)

ENABLE_LANGUAGE(ASM)
SET(SWOOLE_VERSION 4.5.3-alpha)

set(CMAKE_CXX_STANDARD 11)
CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

set(SWOOLE_CLFLAGS pthread dl)

if (APPLE)
    set(CMAKE_SHARED_LINKER_FLAGS "-undefined dynamic_lookup")
else()
    list(APPEND SWOOLE_CLFLAGS rt crypt)
endif()

SET(CMAKE_BUILD_TYPE Debug)

file(GLOB_RECURSE HEAD_FILES FOLLOW_SYMLINKS include/*.h)
file(GLOB_RECURSE HEAD_WAPPER_FILES FOLLOW_SYMLINKS include/wrapper/*.hpp)

SET(LIBRARY_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/lib)
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

add_definitions(-DHAVE_CONFIG_H)

include_directories(BEFORE ./include ./include/wrapper ./)
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

# find OpenSSL
if (DEFINED openssl_dir) 
    include_directories(BEFORE ${openssl_dir}/include)
    link_directories(${openssl_dir}/lib)
else()
    find_package(OpenSSL)
    if (${OPENSSL_FOUND})
        message(STATUS "Found OpenSSL")
        include_directories(BEFORE ${OPENSSL_INCLUDE_DIR})
        list(APPEND SWOOLE_CLFLAGS ${OPENSSL_LIBRARIES})
    else()
        message(STATUS "Not found OpenSSL")
    endif()
endif()

if (DEFINED brotli_dir) 
    include_directories(BEFORE ${brotli_dir}/include)
    link_directories(${brotli_dir}/lib)
endif()

add_subdirectory(src)
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/lib)

execute_process(COMMAND php-config --includes OUTPUT_VARIABLE PHP_INCLUDES OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND php-config --extension-dir OUTPUT_VARIABLE PHP_EXTENSION_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${PHP_INCLUDES}")

# test_server
set(TEST_SRC_LIST examples/cpp/test_server.cc)
add_executable(test_server ${TEST_SRC_LIST})
add_dependencies(test_server lib-swoole)
target_link_libraries(test_server ${SWOOLE_CLFLAGS} swoole)

# co
set(TEST_SRC_LIST examples/cpp/co.cc)
add_executable(co ${TEST_SRC_LIST})
add_dependencies(co lib-swoole)
target_link_libraries(co ${SWOOLE_CLFLAGS} swoole)

# ext-swoole
file(GLOB ext_cxx_files *.cc)
set(ext_src_list  ${ext_cxx_files}
        thirdparty/php/sockets/multicast.cc
        thirdparty/php/sockets/sendrecvmsg.cc
        thirdparty/php/sockets/conversions.cc
        thirdparty/php/sockets/sockaddr_conv.cc
        thirdparty/php/standard/proc_open.cc
        thirdparty/swoole_http_parser.c
        thirdparty/multipart_parser.c
        thirdparty/hiredis/async.c
        thirdparty/hiredis/hiredis.c
        thirdparty/hiredis/net.c
        thirdparty/hiredis/read.c
        thirdparty/hiredis/sds.c
        thirdparty/nghttp2/nghttp2_hd.c
        thirdparty/nghttp2/nghttp2_rcbuf.c
        thirdparty/nghttp2/nghttp2_helper.c
        thirdparty/nghttp2/nghttp2_buf.c
        thirdparty/nghttp2/nghttp2_mem.c
        thirdparty/nghttp2/nghttp2_hd_huffman.c
        thirdparty/nghttp2/nghttp2_hd_huffman_data.c
    )
add_library(ext-swoole SHARED ${ext_src_list})
set_target_properties(ext-swoole PROPERTIES PREFIX "")
set_target_properties(ext-swoole PROPERTIES OUTPUT_NAME "swoole")
target_compile_options(ext-swoole PRIVATE -Wall -g)
add_dependencies(ext-swoole lib-swoole)

if (CMAKE_SYSTEM_NAME MATCHES "Linux")
    execute_process(COMMAND ldconfig -p OUTPUT_VARIABLE LDCONFIG_LIST OUTPUT_STRIP_TRAILING_WHITESPACE)
    #message(STATUS LDCONFIG_LIST)
    if (LDCONFIG_LIST MATCHES brotlienc)
        set(SWOOLE_CLFLAGS "${SWOOLE_CLFLAGS} -l brotlienc")
    endif()

    if (LDCONFIG_LIST MATCHES brotlidec)
        set(SWOOLE_CLFLAGS "${SWOOLE_CLFLAGS} -l brotlidec")
    endif()
endif()

target_link_libraries(ext-swoole ${SWOOLE_CLFLAGS} swoole)

message(STATUS "summary of build options:
    Install prefix:  ${CMAKE_INSTALL_PREFIX}
    Target system:   ${CMAKE_SYSTEM_NAME}
    Compiler:
      C compiler:    ${CMAKE_C_COMPILER}
      CXX compiler:  ${CMAKE_CXX_COMPILER}
      CXXFLAGS:      ${CMAKE_CXX_FLAGS}
")

#install
INSTALL(CODE "MESSAGE(\"Are you run command using root user?\")")
INSTALL(TARGETS ext-swoole LIBRARY DESTINATION ${PHP_EXTENSION_DIR})
INSTALL(TARGETS lib-swoole LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)
INSTALL(FILES ${HEAD_FILES} DESTINATION include/swoole)
INSTALL(FILES ${HEAD_WAPPER_FILES} DESTINATION include/swoole/wrapper)
INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/config.h DESTINATION include/swoole)
