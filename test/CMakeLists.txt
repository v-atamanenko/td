if ((CMAKE_MAJOR_VERSION LESS 3) OR (CMAKE_VERSION VERSION_LESS "3.0.2"))
  message(FATAL_ERROR "CMake >= 3.0.2 is required")
endif()

if (POLICY CMP0065)
  # fix "arm-vita-eabi-g++ is not able to compile a simple test program." https://github.com/tdlib/td/issues/1558
  cmake_policy(SET CMP0065 NEW)
endif()

if (__vita__)
  if(NOT DEFINED ENV{VITASDK})
    message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
  endif()

  set(VITASDK "$ENV{VITASDK}")

  include("${VITASDK}/share/vita.toolchain.cmake" REQUIRED)
  include("${VITASDK}/share/vita.cmake" REQUIRED)

  set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0")
  set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -fpermissive")

  set(VITA_APP_NAME "TdLib Tests")
  set(VITA_TITLEID  "TDLIB0TST")
  set(VITA_VERSION  "01.00")
endif()

#SOURCE SETS
set(TD_TEST_SOURCE
  ${CMAKE_CURRENT_SOURCE_DIR}/country_info.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/db.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/http.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/link.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/message_entities.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/mtproto.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/poll.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/secret.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/secure_storage.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/set_with_position.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/string_cleaning.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/tdclient.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/tqueue.cpp

  ${CMAKE_CURRENT_SOURCE_DIR}/data.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/data.h

  ${TDUTILS_TEST_SOURCE}
  ${TDACTOR_TEST_SOURCE}
)

if (__vita__)
  set(TD_TEST_SOURCE
    ${CMAKE_CURRENT_SOURCE_DIR}/../vitaports/inet.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../vitaports/td_missing_func_placeholder.cpp
    ${TD_TEST_SOURCE}
  )

  set(VITA_MKSFOEX_FLAGS "-d ATTRIBUTE2=12")
endif()

set(TD_TEST_SOURCE ${TD_TEST_SOURCE} PARENT_SCOPE)

set(TESTS_MAIN
  main.cpp
)

#add_library(all_tests STATIC ${TD_TEST_SOURCE})
#target_include_directories(all_tests PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
#target_link_libraries(all_tests PRIVATE tdcore tdclient)

if ((NOT CMAKE_CROSSCOMPILING OR EMSCRIPTEN) OR __vita__)
  #Tests
  if (OPENSSL_FOUND)
    add_executable(test-crypto EXCLUDE_FROM_ALL ${CMAKE_CURRENT_SOURCE_DIR}/crypto.cpp)
    target_include_directories(test-crypto SYSTEM PRIVATE ${OPENSSL_INCLUDE_DIR})
    target_link_libraries(test-crypto PRIVATE ${OPENSSL_CRYPTO_LIBRARY} ${CMAKE_DL_LIBS} ${ZLIB_LIBRARIES} tdutils tdcore)

    if (WIN32)
      if (MINGW)
        target_link_libraries(test-crypto PRIVATE ws2_32 mswsock crypt32)
      else()
        target_link_libraries(test-crypto PRIVATE ws2_32 Mswsock Crypt32)
      endif()
    endif()
  endif()

  add_executable(test-tdutils EXCLUDE_FROM_ALL ${TESTS_MAIN} ${TDUTILS_TEST_SOURCE})
  add_executable(test-online EXCLUDE_FROM_ALL online.cpp)
  add_executable(run_all_tests ${TESTS_MAIN} ${TD_TEST_SOURCE})
  if (CLANG AND NOT CYGWIN AND NOT EMSCRIPTEN AND NOT (CMAKE_SYSTEM_NAME MATCHES "OpenBSD") AND NOT (CMAKE_SIZEOF_VOID_P EQUAL 4))
    target_compile_options(test-tdutils PUBLIC -fsanitize=undefined -fno-sanitize=vptr)
    target_compile_options(run_all_tests PUBLIC -fsanitize=undefined -fno-sanitize=vptr)
    target_compile_options(test-online PUBLIC -fsanitize=undefined -fno-sanitize=vptr)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=undefined -fno-sanitize=vptr")
  endif()
  target_include_directories(run_all_tests PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
  target_include_directories(test-tdutils PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
  target_link_libraries(test-tdutils PRIVATE tdutils)
  target_link_libraries(run_all_tests PRIVATE tdcore tdclient)
  target_link_libraries(test-online PRIVATE tdcore tdclient tdutils tdactor)

  if (__vita__)
    target_link_libraries(test-tdutils PRIVATE
          SceNet_stub
          SceNetCtl_stub
          SceHttp_stub
          SceSsl_stub
          SceSysmodule_stub
          SceAppMgr_stub
          SceAppUtil_stub)
    target_link_libraries(run_all_tests PRIVATE
          SceNet_stub
          SceNetCtl_stub
          SceHttp_stub
          SceSsl_stub
          SceSysmodule_stub
          SceAppMgr_stub
          SceAppUtil_stub)
    target_link_libraries(test-online PRIVATE
          SceNet_stub
          SceNetCtl_stub
          SceHttp_stub
          SceSsl_stub
          SceSysmodule_stub
          SceAppMgr_stub
          SceAppUtil_stub)
  endif()

  if (CLANG)
#    add_executable(fuzz_url fuzz_url.cpp)
#    target_link_libraries(fuzz_url PRIVATE tdcore)
#    target_compile_options(fuzz_url PRIVATE "-fsanitize-coverage=trace-pc-guard")
  endif()

  if (NOT __vita__)
    add_test(run_all_tests run_all_tests)
  else()
    set( VITA_ELF_CREATE    "${VITASDK}/bin/vita-elf-create${TOOL_OS_SUFFIX}"       CACHE PATH "vita-elf-create" )
    set( VITA_ELF_EXPORT    "${VITASDK}/bin/vita-elf-export${TOOL_OS_SUFFIX}"       CACHE PATH "vita-elf-export" )
    set( VITA_LIBS_GEN      "${VITASDK}/bin/vita-libs-gen${TOOL_OS_SUFFIX}"         CACHE PATH "vita-libs-gen" )
    set( VITA_MAKE_FSELF    "${VITASDK}/bin/vita-make-fself${TOOL_OS_SUFFIX}"       CACHE PATH "vita-make-fself" )
    set( VITA_MKSFOEX       "${VITASDK}/bin/vita-mksfoex${TOOL_OS_SUFFIX}"          CACHE PATH "vita-mksfoex" )
    set( VITA_PACK_VPK      "${VITASDK}/bin/vita-pack-vpk${TOOL_OS_SUFFIX}"         CACHE PATH "vita-pack-vpk" )

    vita_create_self(eboot.bin run_all_tests UNSAFE)

    vita_create_vpk(${PROJECT_NAME}.vpk ${VITA_TITLEID} eboot.bin
            VERSION ${VITA_VERSION}
            NAME ${VITA_APP_NAME}
            FILE ${CMAKE_CURRENT_SOURCE_DIR}/../vitaports/tdtest_livearea/icon0.png sce_sys/icon0.png
            FILE ${CMAKE_CURRENT_SOURCE_DIR}/../vitaports/tdtest_livearea/bg.png sce_sys/livearea/contents/bg.png
            FILE ${CMAKE_CURRENT_SOURCE_DIR}/../vitaports/tdtest_livearea/startup.png sce_sys/livearea/contents/startup.png
            FILE ${CMAKE_CURRENT_SOURCE_DIR}/../vitaports/tdtest_livearea/template.xml sce_sys/livearea/contents/template.xml)
  endif()

endif()
