// Copyright (c) 2011-2016 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>

namespace Tools
{
  class PasswordContainer
  {
  public:
    static const size_t max_password_size = 1024;

    PasswordContainer();
    PasswordContainer(std::string&& password);
    PasswordContainer(PasswordContainer&& rhs);
    ~PasswordContainer();

    void clear();
    bool empty() const { return m_empty; }
    const std::string& password() const { return m_password; }
    void password(std::string&& val) { m_password = std::move(val); m_empty = false; }
    bool read_password();
    bool read_password(bool verify, std::string msg);

    bool read_and_validate(std::string msg);

  private:
    bool read_from_file();
    bool read_from_tty();
    bool read_from_tty(std::string& password);

  private:
    bool m_empty;
    std::string m_password;
  };
}
