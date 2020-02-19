#pragma once

#include <string>

#include "common/singleton/const_singleton.h"

namespace Envoy {
namespace Extensions {
namespace Retry {
namespace Header {

/**
 * Well-known retry header names.
 */
class RetryHeadersNameValues {
public:
  const std::string ResponseHeaderRetryHeader = "envoy.retry_header.response_headers";
};

using RetryHeadersValues = ConstSingleton<RetryHeadersNameValues>;

} // namespace Header
} // namespace Retry
} // namespace Extensions
} // namespace Envoy
