#pragma once

#include "envoy/http/codec.h"
#include "envoy/network/connection.h"
#include "envoy/stats/scope.h"

#include "common/buffer/watermark_buffer.h"
#include "common/common/hex.h"
#include "common/common/assert.h"
#include "common/common/to_lower_table.h"
#include "common/http/http1/codec_impl.h"
#include "common/http/http2/codec_impl.h"
#include "common/http/codec_helper.h"
#include "common/http/codes.h"
#include "nghttp2/nghttp2.h"

namespace Envoy {
namespace Http {
namespace Auto {

class UpgradeCallback {
public:
  virtual ~UpgradeCallback() = default;

  virtual void upgrade() PURE;
};

class Http1ServerConnectionImpl : public Http1::ServerConnectionImpl {
public:
  Http1ServerConnectionImpl(Network::Connection& connection, Stats::Scope& stats,
                            ServerConnectionCallbacks& callbacks, Http1Settings settings,
                            uint32_t max_request_headers_kb,
                            const uint32_t max_request_headers_count, UpgradeCallback& callback)
      : Http1::ServerConnectionImpl(connection, stats, callbacks, settings, max_request_headers_kb,
                                    max_request_headers_count),
        callback_(callback) {}

private:
  int onH2cUpgrade(const HeaderMapImplPtr&) override {
    callback_.upgrade();
    return 2;
  }

  UpgradeCallback& callback_;
};

class Http2ServerConnectionImpl : public Http2::ServerConnectionImpl {
public:
  Http2ServerConnectionImpl(Network::Connection& connection, ServerConnectionCallbacks& callbacks,
                            Stats::Scope& scope, const Http2Settings& http2_settings,
                            const uint32_t max_request_headers_kb,
                            const uint32_t max_request_headers_count,
                            const absl::string_view settings_payload, const int head_request)
      : Http2::ServerConnectionImpl(connection, callbacks, scope, http2_settings,
                                    max_request_headers_kb, max_request_headers_count) {
    std::cout << Hex::encode(reinterpret_cast<const uint8_t*>(settings_payload.data()),
                             settings_payload.length())
              << std::endl;
    int rc = nghttp2_session_upgrade2(session_,
                                      reinterpret_cast<const uint8_t*>(settings_payload.data()),
                                      settings_payload.length(), head_request, nullptr);
    ENVOY_LOG(trace, "xydebug: upgraded");
    ASSERT(rc == 0);
  }
};

class ServerConnectionImpl : public ServerConnection,
                             public UpgradeCallback,
                             protected Logger::Loggable<Logger::Id::http> {
public:
  ServerConnectionImpl(Network::Connection& connection, ServerConnectionCallbacks& callbacks,
                       Stats::Scope& stats, const Http1Settings& http1_settings,
                       const Http2Settings& http2_settings, const uint32_t max_request_headers_kb,
                       const uint32_t max_request_headers_count);

public:
  // Http::Connection
  void dispatch(Buffer::Instance& data) override;
  void goAway() override;
  Protocol protocol() override;
  void shutdownNotice() override;
  bool wantsToWrite() override;
  void onUnderlyingConnectionAboveWriteBufferHighWatermark() override;
  void onUnderlyingConnectionBelowWriteBufferLowWatermark() override;

  // UpgradeCallback
  void upgrade() override;

private:
  std::string buildHttp2SettingsFrame();

  Network::Connection& connection_;
  ServerConnectionCallbacks& callbacks_;
  Stats::Scope& stats_;
  const Http2Settings& http2_settings_;
  const uint32_t max_request_headers_kb_;
  const uint32_t max_request_headers_count_;

  ServerConnection* current_server_connection_{nullptr};
  ServerConnectionPtr http1_server_connection_{nullptr}, http2_server_connection_{nullptr};
};

} // namespace Auto
} // namespace Http
} // namespace Envoy
