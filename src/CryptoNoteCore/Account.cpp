// Copyright (c) 2011-2016 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "Account.h"
#include "CryptoNoteSerialization.h"
#include <crypto/crypto.h>
#include <crypto/keccak.h>

namespace CryptoNote {
//-----------------------------------------------------------------
AccountBase::AccountBase() {
  setNull();
}
//-----------------------------------------------------------------
void AccountBase::setNull() {
  m_keys = AccountKeys();
}
//-----------------------------------------------------------------
void AccountBase::generate() {
  Crypto::generate_keys(m_keys.address.spendPublicKey, m_keys.spendSecretKey);

  /* We derive the view secret key by taking our spend secret key, hashing
     with keccak-256, and then using this as the seed to generate a new set
     of keys - the public and private view keys. See generate_keys_from_seed */
  generateViewFromSpend(m_keys.spendSecretKey, m_keys.viewSecretKey, m_keys.address.viewPublicKey);

  //Crypto::generate_keys(m_keys.address.viewPublicKey, m_keys.viewSecretKey);

  m_creation_timestamp = time(NULL);
}

void AccountBase::generateViewFromSpend(Crypto::SecretKey &spend, Crypto::SecretKey &viewSecret, Crypto::PublicKey &viewPublic) {
  Crypto::SecretKey viewKeySeed;

  //keccak((uint8_t const*)&spend, (int)sizeof(spend), (uint8_t *)&viewKeySeed, (int)sizeof(viewKeySeed));

  Crypto::generate_keys_from_seed(viewPublic, viewSecret, viewKeySeed);
}

void AccountBase::generateViewFromSpend(Crypto::SecretKey &spend, Crypto::SecretKey &viewSecret) {
  /* If we don't need the pub key */
  Crypto::PublicKey unused_dummy_variable;
  generateViewFromSpend(spend, viewSecret, unused_dummy_variable);
}

//-----------------------------------------------------------------
const AccountKeys &AccountBase::getAccountKeys() const {
  return m_keys;
}

void AccountBase::setAccountKeys(const AccountKeys &keys) {
  m_keys = keys;
}
//-----------------------------------------------------------------

void AccountBase::serialize(ISerializer &s) {
  s(m_keys, "m_keys");
  s(m_creation_timestamp, "m_creation_timestamp");
}
}
