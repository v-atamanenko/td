//
// Copyright Aliaksei Levin (levlam@telegram.org), Arseny Smirnov (arseny30@gmail.com) 2014-2021
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once

#include "td/telegram/net/NetActor.h"
#include "td/telegram/net/NetQuery.h"
#include "td/telegram/SendCodeHelper.h"
#include "td/telegram/td_api.h"
#include "td/telegram/telegram_api.h"

#include "td/actor/actor.h"

#include "td/utils/common.h"
#include "td/utils/Status.h"

namespace td {

class PhoneNumberManager final : public NetActor {
 public:
  enum class Type : int32 { ChangePhone, VerifyPhone, ConfirmPhone };
  PhoneNumberManager(Type type, ActorShared<> parent);
  void get_state(uint64 query_id);

  using Settings = td_api::object_ptr<td_api::phoneNumberAuthenticationSettings>;

  void set_phone_number(uint64 query_id, string phone_number, Settings settings);
  void set_phone_number_and_hash(uint64 query_id, string hash, string phone_number, Settings settings);

  void resend_authentication_code(uint64 query_id);
  void check_code(uint64 query_id, string code);

 private:
  Type type_;

  enum class State : int32 { Ok, WaitCode } state_ = State::Ok;
  enum class NetQueryType : int32 { None, SendCode, CheckCode };

  ActorShared<> parent_;
  uint64 query_id_ = 0;
  uint64 net_query_id_ = 0;
  NetQueryType net_query_type_;

  SendCodeHelper send_code_helper_;

  void on_new_query(uint64 query_id);

  void on_query_ok();

  void on_query_error(Status status);

  static void on_query_error(uint64 id, Status status);

  void start_net_query(NetQueryType net_query_type, NetQueryPtr net_query);

  void send_new_send_code_query(uint64 query_id, const telegram_api::Function &query);

  void send_new_check_code_query(const telegram_api::Function &query);

  void process_check_code_result(Result<tl_object_ptr<telegram_api::User>> &&result);

  void process_check_code_result(Result<bool> &&result);

  void on_result(NetQueryPtr result) final;

  void on_send_code_result(NetQueryPtr &result);

  void on_check_code_result(NetQueryPtr &result);

  void tear_down() final;
};

}  // namespace td
