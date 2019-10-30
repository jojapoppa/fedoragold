// Copyright (c) 2011-2016 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <memory>

#include <Common/Base64.h>
#include <HTTP/HttpRequest.h>
#include <HTTP/HttpResponse.h>
#include <System/TcpConnection.h>
#include <System/TcpStream.h>
#include "JsonRpc.h"

#include "Logging/LoggerRef.h"

#include "Serialization/SerializationTools.h"

namespace CryptoNote {

class ConnectException : public std::runtime_error  {
public:
  ConnectException(const std::string& whatArg);
};

class HttpClient {
public:

  HttpClient(System::Dispatcher& dispatcher, const std::string& address, uint16_t port, Logging::LoggerRef&);
  HttpClient(System::Dispatcher& dispatcher, const std::string& address, uint16_t port, Logging::ILogger&);
  ~HttpClient();
  void request(const HttpRequest& req, HttpResponse& res);
  
  bool isConnected() const;

private:
  void connect();
  void disconnect();

  const std::string m_address;
  const uint16_t m_port;

  Logging::LoggerRef m_logger;

  bool m_connected = false;
  System::Dispatcher& m_dispatcher;
  System::TcpConnection m_connection;
  std::unique_ptr<System::TcpStreambuf> m_streamBuf;
};

template <typename Request, typename Response>
void invokeJsonCommand(HttpClient& client, const std::string& url, const Request& req, Response& res, const std::string& user = "", const std::string& password = "") {
  HttpRequest hreq;
  HttpResponse hres;

  hreq.addHeader("Content-Type", "application/json");
  if (!user.empty() || !password.empty()) {
    hreq.addHeader("Authorization", "Basic " + Tools::Base64::encode(user + ":" + password));
  }

  hreq.setUrl(url);
  hreq.setBody(storeToJson(req));
  client.request(hreq, hres);

  if (hres.getStatus() != HttpResponse::STATUS_200) {
    throw std::runtime_error("HTTP status: " + std::to_string(hres.getStatus()));
  }

  if (!loadFromJson(res, hres.getBody())) {
    throw std::runtime_error("Failed to parse JSON response");
  }
}

template <typename Request, typename Response>
void invokeJsonRpcCommand(HttpClient& client, const std::string& method, const Request& req, Response& res, const std::string& user = "", const std::string& password = "") {
  try {

    JsonRpc::JsonRpcRequest jsReq;

    jsReq.setMethod(method);
    jsReq.setParams(req);

    HttpRequest httpReq;
    HttpResponse httpRes;

    httpReq.addHeader("Content-Type", "application/json");
    if (!user.empty() || !password.empty()) {
      httpReq.addHeader("Authorization", "Basic " + Tools::Base64::encode(user + ":" + password));
    }
    httpReq.setUrl("/json_rpc");
    httpReq.setBody(jsReq.getBody());

    client.request(httpReq, httpRes);

    JsonRpc::JsonRpcResponse jsRes;

    //if (httpRes.getStatus() == HttpResponse::STATUS_200) {
      jsRes.parse(httpRes.getBody());
      if (!jsRes.getResult(res)) {
        throw std::runtime_error("HTTP status: " + std::to_string(httpRes.getStatus()));
      }
    //}

  } catch (const ConnectException&) {
    throw std::runtime_error("HTTP status: CONNECT_ERROR");
  } catch (const std::exception&) {
    throw std::runtime_error("HTTP status: NETWORK_ERROR");
  }
}

template <typename Request, typename Response>
void invokeBinaryCommand(HttpClient& client, const std::string& url, const Request& req, Response& res, const std::string& user = "", const std::string& password = "") {
  HttpRequest hreq;
  HttpResponse hres;

  if (!user.empty() || !password.empty()) {
    hreq.addHeader("Authorization", "Basic " + Tools::Base64::encode(user + ":" + password));
  }
  hreq.setUrl(url);
  hreq.setBody(storeToBinaryKeyValue(req));
  client.request(hreq, hres);
  if (!loadFromBinaryKeyValue(res, hres.getBody())) {
    throw std::runtime_error("Failed to parse binary response");
  }
}

}
