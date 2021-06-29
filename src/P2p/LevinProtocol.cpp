// Copyright (c) 2011-2016 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "LevinProtocol.h"
#include "P2pProtocolDefinitions.h"
#include "System/TcpConnection.h"

using namespace CryptoNote;

namespace {

const uint64_t LEVIN_SIGNATURE = 0x0101010101012101LL;  //Bender's nightmare
const uint32_t LEVIN_PACKET_REQUEST = 0x00000001;
const uint32_t LEVIN_PACKET_RESPONSE = 0x00000002;
const uint32_t LEVIN_DEFAULT_MAX_PACKET_SIZE = 100000000;      //100MB by default
const uint32_t LEVIN_PROTOCOL_VER_1 = 1;

#pragma pack(push)
#pragma pack(1)
struct bucket_head2
{
  uint64_t m_signature;
  uint64_t m_cb;
  bool     m_have_to_return_data;
  uint32_t m_command;
  int32_t  m_return_code;
  uint32_t m_flags;
  uint32_t m_protocol_version;
};
#pragma pack(pop)

}

bool LevinProtocol::Command::needReply() const {
  return !(isNotify || isResponse);
}

LevinProtocol::LevinProtocol(System::TcpConnection& connection) 
  : m_conn(connection) {}

void LevinProtocol::sendMessage(uint32_t command, const BinaryArray& out, bool needResponse, Logging::LoggerRef& logger) {
  bucket_head2 head = { 0 };
  head.m_signature = LEVIN_SIGNATURE;
  head.m_cb = out.size();
  head.m_have_to_return_data = needResponse;
  head.m_command = command;
  head.m_protocol_version = LEVIN_PROTOCOL_VER_1;
  head.m_flags = LEVIN_PACKET_REQUEST;

  // write header and body in one operation
  BinaryArray writeBuffer;

  logger(DEBUGGING) << "LevinProtocol sendMessage: " << command << " size: " << "(" << sizeof(head) + out.size() << ") " <<  "max: " << writeBuffer.max_size(); 

  writeBuffer.reserve(sizeof(head) + out.size());

  Common::VectorOutputStream stream(writeBuffer);
  stream.writeSome(&head, sizeof(head));
  stream.writeSome(out.data(), out.size());

  writeStrict(writeBuffer.data(), writeBuffer.size(), logger);

  logger(DEBUGGING) << "sendMessage completed";
}

bool LevinProtocol::readCommand(Command& cmd, Logging::LoggerRef& logger) {
  bucket_head2 head = { 0 };

  if (!readStrict(reinterpret_cast<uint8_t*>(&head), sizeof(head), logger, false)) {
    // Windows OS network fails under high volume, allow it to retry headers...
    cmd.command = COMMAND_BLOCKED::ID;
    return false;
  }

  if (head.m_signature != LEVIN_SIGNATURE) {
    logger(DEBUGGING) << "Levin signature mismatch";
    return false;
  }

  if (head.m_cb > LEVIN_DEFAULT_MAX_PACKET_SIZE) {
    logger(DEBUGGING) << "Levin packet size is too big";
    return false;
  }

  BinaryArray buf;

  if (head.m_cb != 0) {
    buf.resize(head.m_cb);
    if (!readStrict(&buf[0], head.m_cb, logger)) {
      logger(DEBUGGING) << "Levin failed to read in buffer!";
      return false;
    }
  }

  cmd.command = head.m_command;
  cmd.buf = std::move(buf);
  cmd.isNotify = !head.m_have_to_return_data;
  cmd.isResponse = (head.m_flags & LEVIN_PACKET_RESPONSE) == LEVIN_PACKET_RESPONSE;

  return true;
}

void LevinProtocol::sendReply(uint32_t command, const BinaryArray& out, int32_t returnCode, Logging::LoggerRef& logger) {
  bucket_head2 head = { 0 };
  head.m_signature = LEVIN_SIGNATURE;
  head.m_cb = out.size();
  head.m_have_to_return_data = false;
  head.m_command = command;
  head.m_protocol_version = LEVIN_PROTOCOL_VER_1;
  head.m_flags = LEVIN_PACKET_RESPONSE;
  head.m_return_code = returnCode;

  BinaryArray writeBuffer;
  writeBuffer.reserve(sizeof(head) + out.size());

  Common::VectorOutputStream stream(writeBuffer);
  stream.writeSome(&head, sizeof(head));
  stream.writeSome(out.data(), out.size());

  writeStrict(writeBuffer.data(), writeBuffer.size(), logger);
}

void LevinProtocol::writeStrict(const uint8_t* ptr, size_t size, Logging::LoggerRef &logger) {
  size_t offset = 0;
  while (offset < size) {
    //logger(DEBUGGING) << "LevinProtocol writeStrict offset/size: " << offset << "/" << size;
    offset += m_conn.write(ptr + offset, size - offset, logger);
  }
}

bool LevinProtocol::readStrict(uint8_t* ptr, size_t size, Logging::LoggerRef &logger, bool bSynchronous) {
  size_t offset = 0;
  //logger(DEBUGGING) << "readStrict is looking for these many bytes: " << size;
  
  while (offset < size) {

    // bSyncronous
    size_t read = m_conn.read(ptr + offset, size - offset, logger);

    //logger(DEBUGGING) << "readStrict just got some bytes: " << read;

    if (read == 0) {
      //logger(DEBUGGING) << "readStrict found no more input bytes, found: " << offset;
      return false;
    }

    offset += read;
  }

  //logger(DEBUGGING) << "readStrict succeeded and found: " << offset;
  return true;
}
