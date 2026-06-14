#pragma once

#include <optional>
#include <string>
#include <vector>

#include "types.hpp"

namespace duplexprint::platform {

class ProfileStore {
 public:
  virtual ~ProfileStore() = default;

  [[nodiscard]] virtual std::vector<core::PrinterProfile> list() const = 0;
  [[nodiscard]] virtual std::optional<core::PrinterProfile> get(const std::string& printer_name) const = 0;
  virtual void save(const core::PrinterProfile& profile) = 0;
};

}  // namespace duplexprint::platform
