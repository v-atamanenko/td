//
// Copyright Aliaksei Levin (levlam@telegram.org), Arseny Smirnov (arseny30@gmail.com) 2014-2021
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include "td/utils/common.h"
#include "td/utils/crypto.h"
#include "td/utils/logging.h"
#include "td/utils/OptionParser.h"
#include "td/utils/Slice.h"
#include "td/utils/tests.h"

#if TD_EMSCRIPTEN
#include <emscripten.h>
#endif

#ifdef __vita__
#include <psp2/kernel/processmgr.h>
#include <psp2/sysmodule.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/libssl.h>
#include <psp2/net/http.h>
#include <psp2/net/net.h>
#include <psp2/net/netctl.h>

#include <tdutils/td/utils/port/path.h>

int _newlib_heap_size_user = 128 * 1024 * 1024;
#endif

int main(int argc, char **argv) {
#ifdef __vita__
  int ret;

  if ((ret = sceSysmoduleLoadModule(SCE_SYSMODULE_NET)) < 0) {
    fprintf(stderr, "SCE_SYSMODULE_NET loading failed: %x\n", ret);
    return 1;
  }

  SceNetInitParam param;
  static char memory[8 * 1024 * 1024];
  param.memory = memory;
  param.size = sizeof(memory);
  param.flags = 0;

  if ((ret = sceNetInit(&param)) < 0) {
    fprintf(stderr, "sceNetInit failed: %x\n", ret);
    return 1;
  }

  if ((ret = sceNetCtlInit()) < 0) {
    fprintf(stderr, "sceNetCtlInit failed %x\n", ret);
    return 1;
  }

  if ((ret = sceSysmoduleLoadModule(SCE_SYSMODULE_HTTP)) < 0) {
    fprintf(stderr, "SCE_SYSMODULE_HTTP loading failed: %x\n", ret);
    return 1;
  }

  if ((ret = sceHttpInit(1024 * 1024)) < 0) {
    fprintf(stderr, "sceHttpInit failed: %x\n", ret);
    return 1;
  }

  if ((ret = sceSysmoduleLoadModule(SCE_SYSMODULE_HTTPS)) < 0) {
    fprintf(stderr, "SCE_SYSMODULE_HTTPS loading failed: %x\n", ret);
    return 1;
  }

  if ((ret = sceSysmoduleLoadModule(SCE_SYSMODULE_SSL)) < 0) {
    fprintf(stderr, "SCE_SYSMODULE_SSL loading failed: %x\n", ret);
    return 1;
  }

  if ((ret = sceSslInit(1024 * 1024)) < 0) {
    fprintf(stderr, "SslInit failed: %x\n", ret);
    return 1;
  }

  printf("Network initialized.\n");

  td::CSlice main_dir = "ux0:data/td_test";
  td::rmrf(main_dir).ignore();
  td::mkdir(main_dir, 0777).ensure();

  printf("Tempdir %s is ready.\n", main_dir.c_str());
#endif

  td::init_openssl_threads();

  td::TestsRunner &runner = td::TestsRunner::get_default();
  SET_VERBOSITY_LEVEL(VERBOSITY_NAME(ERROR));

#ifdef __vita__
  // Use this to run specific tests
  //const char* n_argv[] = { "main", "-f", "Mtproto_handshake" };
  //argv = n_argv;
  //argc = 3;
#endif

  td::OptionParser options;
  options.add_option('f', "filter", "Run only specified tests",
                     [&](td::Slice filter) { runner.add_substr_filter(filter.str()); });
  options.add_option('s', "stress", "Run tests infinitely", [&] { runner.set_stress_flag(true); });
  auto r_non_options = options.run(argc, argv, 0);
  if (r_non_options.is_error()) {
    LOG(PLAIN) << argv[0] << ": " << r_non_options.error().message();
    LOG(PLAIN) << options;
    return 1;
  }

#if TD_EMSCRIPTEN
  emscripten_set_main_loop(
      [] {
        td::TestsRunner &default_runner = td::TestsRunner::get_default();
        if (!default_runner.run_all_step()) {
          emscripten_cancel_main_loop();
        }
      },
      10, 0);
#else
  runner.run_all();
#endif

#ifdef __vita__
  printf("All tests completed. Shutting down.\n");
  sceSysmoduleUnloadModule(SCE_SYSMODULE_SSL);
  sceSysmoduleUnloadModule(SCE_SYSMODULE_HTTPS);
  sceSysmoduleUnloadModule(SCE_SYSMODULE_HTTP);
  sceSysmoduleUnloadModule(SCE_SYSMODULE_NET);
  sceKernelExitProcess(0);
#endif
  return 0;
}
