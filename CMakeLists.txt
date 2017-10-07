cmake_minimum_required(VERSION 2.8.11)
cmake_policy(SET CMP0003 NEW)

project(wqet)

find_package(OpenSSL REQUIRED)
if (OPENSSL_FOUND AND (OPENSSL_VERSION VERSION_LESS "1.0.2"))
    message(FATAL "OpenSSL 1.0.2 or above is missing")
endif ()

include_directories(${OPENSSL_INCLUDE_DIR} deps/picotls/include include)
add_executable(wqet ${PICOTLS_OPENSSL_FILES}
    src/wqet.c
    src/wqet_mruby.c)

target_link_libraries(wqet ${OPENSSL_LIBRARIES} ${CMAKE_DL_LIBS})

######
# fmt
#
find_program(CLANG_FORMAT_BIN clang-format)
if (CLANG_FORMAT_BIN)
    add_custom_target(fmt ${CMAKE_SOURCE_DIR}/misc/clang_format_all.sh ${CLANG_FORMAT_BIN})
endif()

######
# mruby
#
if (${CMAKE_C_COMPILER_ID} STREQUAL "Clang")
    set(MRUBY_TOOLCHAIN "clang")
else ()
    set(MRUBY_TOOLCHAIN "gcc")
endif ()

if (NOT WITH_MRUBY_INCLUDE OR NOT WITH_MRUBY_LIB)
    add_custom_target(mruby MRUBY_TOOLCHAIN=${MRUBY_TOOLCHAIN}
                      MRUBY_CONFIG=${CMAKE_CURRENT_SOURCE_DIR}/misc/mruby_config.rb
                      MRUBY_BUILD_DIR=${CMAKE_CURRENT_BINARY_DIR}/mruby ruby minirake
                      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/deps/mruby)

    TARGET_INCLUDE_DIRECTORIES(wqet BEFORE PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/deps/mruby/include)
    # note: the paths need to be determined before libmruby.flags.mak is generated
    target_link_libraries(wqet "${CMAKE_CURRENT_BINARY_DIR}/mruby/host/lib/libmruby.a")
    add_dependencies(wqet mruby)
else ()
    target_link_libraries(wqet "${WITH_MRUBY_LIB}")
    TARGET_INCLUDE_DIRECTORIES(wqet BEFORE PRIVATE ${WITH_MRUBY_INCLUDE})
endif ()

set(CMAKE_C_FLAGS "-std=c99 -Wall -O2 -g ${CC_WARNING_FLAGS} ${CMAKE_C_FLAGS}")
set(PICOTLS_OPENSSL_FILES
	deps/picotls/lib/openssl.c
	deps/picotls/lib/pembase64.c
	deps/picotls/lib/picotls.c)


