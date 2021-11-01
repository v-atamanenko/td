if ((CMAKE_MAJOR_VERSION LESS 3) OR (CMAKE_VERSION VERSION_LESS "3.0.2"))
  message(FATAL_ERROR "CMake >= 3.0.2 is required")
endif()

if (POLICY CMP0065)
  # fix #1558 "arm-vita-eabi-g++ is not able to compile a simple test program."
  cmake_policy(SET CMP0065 NEW)
endif()

if (__vita__)
  if(NOT DEFINED ENV{VITASDK})
    message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
  endif()

  if(DEFINED ENV{PSVITAIP})
    set(PSVITAIP "$ENV{PSVITAIP}")
  else()
    set(PSVITAIP "192.168.1.14")
    message(WARNING "PSVITAIP environment variable is not set, using default IP ${PSVITAIP}")
  endif()

  set(VITASDK "$ENV{VITASDK}")

  # Temporarily unset CMAKE_CROSSCOMPILING so that the toolchain will load
  set(TEMP_CMAKE_CROSSCOMPILING CMAKE_CROSSCOMPILING)
  unset(CMAKE_CROSSCOMPILING)
  include("${VITASDK}/share/vita.toolchain.force.cmake" REQUIRED)
  include("${VITASDK}/share/vita.cmake" REQUIRED)
  set(CMAKE_CROSSCOMPILING TEMP_CMAKE_CROSSCOMPILING)

  # For debugging add -g -O0
  set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
  set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpermissive")

  set(VITA_APP_NAME "TdLib Tests")
  set(VITA_TITLEID  "TDLIBTEST")
  set(VITA_VERSION  "01.00")

  set(VITA_MKSFOEX_FLAGS "-d ATTRIBUTE2=12")
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
    ${CMAKE_CURRENT_SOURCE_DIR}/../vitaports/td_missing_func_placeholder.cpp
    ${TD_TEST_SOURCE}
  )
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
    target_link_libraries(run_all_tests PRIVATE
          pthread
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

  if (__vita__)
    vita_create_self(eboot.bin run_all_tests UNSAFE)

    vita_create_vpk(${PROJECT_NAME}.vpk ${VITA_TITLEID} eboot.bin
            VERSION ${VITA_VERSION}
            NAME ${VITA_APP_NAME}
            FILE ${CMAKE_CURRENT_SOURCE_DIR}/../vitaports/tdtest_livearea/icon0.png sce_sys/icon0.png
            FILE ${CMAKE_CURRENT_SOURCE_DIR}/../vitaports/tdtest_livearea/bg.png sce_sys/livearea/contents/bg.png
            FILE ${CMAKE_CURRENT_SOURCE_DIR}/../vitaports/tdtest_livearea/startup.png sce_sys/livearea/contents/startup.png
            FILE ${CMAKE_CURRENT_SOURCE_DIR}/../vitaports/tdtest_livearea/template.xml sce_sys/livearea/contents/template.xml)

    add_custom_target(send
            COMMAND echo destroy | nc ${PSVITAIP} 1338
            COMMAND curl -T eboot.bin ftp://${PSVITAIP}:1337/ux0:/app/${VITA_TITLEID}/
            COMMAND echo launch ${VITA_TITLEID} | nc ${PSVITAIP} 1338
            DEPENDS eboot.bin)

    return()
  endif()

  add_test(run_all_tests run_all_tests)

endif()
