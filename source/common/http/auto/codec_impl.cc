#include "common/http/auto/codec_impl.h"

#include "common/common/base64.h"
#include "common/http/http1/codec_impl.h"
#include "common/http/http2/codec_impl.h"

namespace Envoy {
namespace Http {
namespace Auto {

ServerConnectionImpl::ServerConnectionImpl(Network::Connection& connection,
                                           ServerConnectionCallbacks& callbacks,
                                           Stats::Scope& stats, const Http1Settings& http1_settings,
                                           const Http2Settings& http2_settings,
                                           const uint32_t max_request_headers_kb,
                                           const uint32_t max_request_headers_count)
    : connection_(connection), callbacks_(callbacks), stats_(stats),
      http2_settings_(http2_settings), max_request_headers_kb_(max_request_headers_kb),
      max_request_headers_count_(max_request_headers_count),
      http1_server_connection_(std::make_unique<Http1ServerConnectionImpl>(
          connection, stats, callbacks, http1_settings, max_request_headers_kb,
          max_request_headers_count, *this)) {
  current_server_connection_ = http1_server_connection_.get();
}

void ServerConnectionImpl::dispatch(Buffer::Instance& data) {
  current_server_connection_->dispatch(data);
}
void ServerConnectionImpl::goAway() { current_server_connection_->goAway(); }
Protocol ServerConnectionImpl::protocol() { return current_server_connection_->protocol(); }
void ServerConnectionImpl::shutdownNotice() { current_server_connection_->shutdownNotice(); }
bool ServerConnectionImpl::wantsToWrite() { return current_server_connection_->wantsToWrite(); }
void ServerConnectionImpl::onUnderlyingConnectionAboveWriteBufferHighWatermark() {
  current_server_connection_->onUnderlyingConnectionAboveWriteBufferHighWatermark();
}
void ServerConnectionImpl::onUnderlyingConnectionBelowWriteBufferLowWatermark() {
  current_server_connection_->onUnderlyingConnectionBelowWriteBufferLowWatermark();
}

void ServerConnectionImpl::upgrade() {
  ENVOY_LOG(trace, "xydebug: start upgrading");
  std::string settings = Base64Url::decode("AAMAAABkAARAAAAAAAIAAAAA");
  http2_server_connection_ = std::make_unique<Http2ServerConnectionImpl>(
      connection_, callbacks_, stats_, http2_settings_, max_request_headers_kb_,
      max_request_headers_count_, settings, 0);
  current_server_connection_ = http2_server_connection_.get();

  // send protocol switch response.
  // HTTP/1.1 101 Switching Protocols
  // Connection: Upgrade
  // Upgrade: h2c
  //
  // [ HTTP/2 connection ...
  Buffer::OwnedImpl protocol_switch_response(absl::StrCat(
      "HTTP/1.1 101 Switching Protocols\r\nConnection: Upgrade\r\nUpgrade: h2c\r\n\r\n",
      buildHttp2SettingsFrame()));
  connection_.write(protocol_switch_response, false);
}

std::string ServerConnectionImpl::buildHttp2SettingsFrame() {
  ASSERT(http2_settings_.hpack_table_size_ <= Http2Settings::MAX_HPACK_TABLE_SIZE);
  ASSERT(Http2Settings::MIN_MAX_CONCURRENT_STREAMS <= http2_settings_.max_concurrent_streams_ &&
         http2_settings_.max_concurrent_streams_ <= Http2Settings::MAX_MAX_CONCURRENT_STREAMS);
  ASSERT(Http2Settings::MIN_INITIAL_STREAM_WINDOW_SIZE <=
             http2_settings_.initial_stream_window_size_ &&
         http2_settings_.initial_stream_window_size_ <=
             Http2Settings::MAX_INITIAL_STREAM_WINDOW_SIZE);
  ASSERT(Http2Settings::MIN_INITIAL_CONNECTION_WINDOW_SIZE <=
             http2_settings_.initial_connection_window_size_ &&
         http2_settings_.initial_connection_window_size_ <=
             Http2Settings::MAX_INITIAL_CONNECTION_WINDOW_SIZE);

  std::vector<nghttp2_settings_entry> iv;

  if (http2_settings_.allow_connect_) {
    iv.push_back({NGHTTP2_SETTINGS_ENABLE_CONNECT_PROTOCOL, 1});
  }

  if (http2_settings_.hpack_table_size_ != NGHTTP2_DEFAULT_HEADER_TABLE_SIZE) {
    iv.push_back({NGHTTP2_SETTINGS_HEADER_TABLE_SIZE, http2_settings_.hpack_table_size_});
    ENVOY_CONN_LOG(debug, "setting HPACK table size to {}", connection_,
                   http2_settings_.hpack_table_size_);
  }

  if (http2_settings_.max_concurrent_streams_ != NGHTTP2_INITIAL_MAX_CONCURRENT_STREAMS) {
    iv.push_back(
        {NGHTTP2_SETTINGS_MAX_CONCURRENT_STREAMS, http2_settings_.max_concurrent_streams_});
    ENVOY_CONN_LOG(debug, "setting max concurrent streams to {}", connection_,
                   http2_settings_.max_concurrent_streams_);
  }

  if (http2_settings_.initial_stream_window_size_ != NGHTTP2_INITIAL_WINDOW_SIZE) {
    iv.push_back(
        {NGHTTP2_SETTINGS_INITIAL_WINDOW_SIZE, http2_settings_.initial_stream_window_size_});
    ENVOY_CONN_LOG(debug, "setting stream-level initial window size to {}", connection_,
                   http2_settings_.initial_stream_window_size_);
  }

  //  The required space in buf for the niv entries is 6*niv bytes and if the given buffer is too
  //  small, an error is returned. The maximum size of niv is 4, reserve 2 * 4 * 6 bytes.
  unsigned char settings[48];
  int len = nghttp2_pack_settings_payload(settings, sizeof(settings), &iv[0], iv.size());
  if (len <= 0) {
    std::cout << "could not pack SETTINGs " << nghttp2_strerror(len) << std::endl;
  }
  std::cout << "len=" << len << std::endl;
  for (int i = 0; i < 48; ++i) {
    std::cout << settings[i] << std::endl;
  }
  std::cout << Base64Url::encode(reinterpret_cast<char*>(settings), len) << std::endl;
  std::string result = std::string(reinterpret_cast<char*>(settings), len);
  std::cout << "WTF" << result << std::endl;
  return Base64Url::encode(reinterpret_cast<char*>(settings), len);
}

} // namespace Auto
} // namespace Http
} // namespace Envoy
