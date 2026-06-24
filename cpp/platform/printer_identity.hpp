#pragma once

#include <string>

#include "profile_repository.hpp"
#include "types.hpp"

namespace duplexprint::platform {

class PrinterIdentity {
 public:
  [[nodiscard]] static std::string normalize_key(const std::string& manufacturer, const std::string& model);
  [[nodiscard]] static cloud::PrinterFingerprint fingerprint_for(const core::PrinterInfo& printer);
};

}  // namespace duplexprint::platform
