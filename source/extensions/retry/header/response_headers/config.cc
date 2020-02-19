#include "extensions/retry/header/response_headers/config.h"

#include "envoy/config/retry/response_headers/response_headers_config.pb.h"
#include "envoy/config/retry/response_headers/response_headers_config.pb.validate.h"
#include "envoy/registry/registry.h"
#include "envoy/upstream/retry.h"

namespace Envoy {
namespace Extensions {
namespace Retry {
namespace Header {

Upstream::RetryHeaderSharedPtr ResponseHeadersRetryHeaderFactory::createRetryPriority(
    const Protobuf::Message& config, ProtobufMessage::ValidationVisitor& validation_visitor) {
  const auto& config = MessageUtil::downcastAndValidate<
      const envoy::config::retry::response_headerrs::ResponseHeadersConfig&>(config,
                                                                             validation_visitor);

  return std::make_share<ResponseHeadersRetryHeader>(retry_on, retriable_status_codes,
                                                     retriable_headers);
}

REGISTER_FACTORY(ResponseHeadersRetryHeaderFactory, Upstream::RetryHeaderFactory);

} // namespace Header
} // namespace Retry
} // namespace Extensions
} // namespace Envoy
