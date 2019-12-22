// Copyright (c) 2011-2016 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include <system_error>

namespace CryptoNote {
namespace error {

// custom error conditions enum type:
enum WalletErrorCodes {
  NOT_INITIALIZED = 1,
  ALREADY_INITIALIZED,
  WRONG_STATE,
  WRONG_PASSWORD,
  INTERNAL_WALLET_ERROR,
  MIXIN_COUNT_TOO_BIG,
  BAD_ADDRESS,
  TRANSACTION_SIZE_TOO_BIG,
  WRONG_AMOUNT,
  SUM_OVERFLOW,
  ZERO_DESTINATION,
  TX_CANCEL_IMPOSSIBLE,
  TX_CANCELLED,
  OPERATION_CANCELLED,
  TX_TRANSFER_IMPOSSIBLE,
  WRONG_VERSION,
  FEE_TOO_SMALL,
  KEY_GENERATION_ERROR,
  INDEX_OUT_OF_RANGE,
  ADDRESS_ALREADY_EXISTS,
  TRACKING_MODE,
  WRONG_PARAMETERS,
  OBJECT_NOT_FOUND,
  WALLET_NOT_FOUND,
  CHANGE_ADDRESS_REQUIRED,
  CHANGE_ADDRESS_NOT_FOUND,
  DESTINATION_ADDRESS_REQUIRED,
  DESTINATION_ADDRESS_NOT_FOUND
};

// custom category:
class WalletErrorCategory : public std::error_category {
public:
  static WalletErrorCategory INSTANCE;

  virtual const char* name() const throw() override {
    return "WalletErrorCategory";
  }

  virtual std::error_condition default_error_condition(int ev) const throw() override {
    return std::error_condition(ev, *this);
  }

  std::string m_msg;

  virtual std::string message(int ev) const override {
    std::string ss;
    switch (ev) {
    case NOT_INITIALIZED:          ss=m_msg+"Object was not initialized";
    break;
    case WRONG_PASSWORD:           ss=m_msg+"The password is wrong";
    break;
    case ALREADY_INITIALIZED:      ss=m_msg+"The object is already initialized";
    break;
    case INTERNAL_WALLET_ERROR:    ss=m_msg+"Internal error occurred";
    break;
    case MIXIN_COUNT_TOO_BIG:      ss=m_msg+"MixIn count is too big";
    break;
    case BAD_ADDRESS:              ss=m_msg+"Bad address";
    break;
    case TRANSACTION_SIZE_TOO_BIG: ss=m_msg+"Transaction size is too big";
    break;
    case WRONG_AMOUNT:             ss=m_msg+"Wrong amount";
    break;
    case SUM_OVERFLOW:             ss=m_msg+"Sum overflow";
    break;
    case ZERO_DESTINATION:         ss=m_msg+"The destination is empty";
    break;
    case TX_CANCEL_IMPOSSIBLE:     ss=m_msg+"Impossible to cancel transaction";
    break;
    case WRONG_STATE:              ss=m_msg+"The wallet is in wrong state (maybe loading or saving), try again later";
    break;
    case OPERATION_CANCELLED:      ss=m_msg+"The operation you've requested has been cancelled";
    break;
    case TX_TRANSFER_IMPOSSIBLE:   ss=m_msg+"Transaction transfer impossible";
    break;
    case WRONG_VERSION:            ss=m_msg+"Wrong version";
    break;
    case FEE_TOO_SMALL:            ss=m_msg+"Transaction fee is too small";
    break;
    case KEY_GENERATION_ERROR:     ss=m_msg+"Cannot generate new key";
    break;
    case INDEX_OUT_OF_RANGE:       ss=m_msg+"Index is out of range";
    break;
    case ADDRESS_ALREADY_EXISTS:   ss=m_msg+"Address already exists";
    break;
    case TRACKING_MODE:            ss=m_msg+"The wallet is in tracking mode";
    break;
    case WRONG_PARAMETERS:         ss=m_msg+"Wrong parameters passed";
    break;
    case OBJECT_NOT_FOUND:         ss=m_msg+"Object not found";
    break;
    case WALLET_NOT_FOUND:         ss=m_msg+"Requested wallet not found";
    break;
    case CHANGE_ADDRESS_REQUIRED:  ss=m_msg+"Change address required";
    break;
    case CHANGE_ADDRESS_NOT_FOUND: ss=m_msg+"Change address not found";
    break;
    default:                       ss=m_msg+"Unknown error";
    break;
    }

    return ss;
  }

  WalletErrorCategory() {
  }
};

}
}

inline std::error_code make_error_code(CryptoNote::error::WalletErrorCodes e, std::string ermsg="") {
  CryptoNote::error::WalletErrorCategory::INSTANCE.m_msg= ermsg;
  return std::error_code(static_cast<int>(e), CryptoNote::error::WalletErrorCategory::INSTANCE);
}

namespace std {

template <>
struct is_error_code_enum<CryptoNote::error::WalletErrorCodes>: public true_type {};

}
