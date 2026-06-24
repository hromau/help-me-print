#pragma once

#include <optional>
#include <string>
#include <vector>

#include "types.hpp"

namespace duplexprint::cloud {

struct PrinterFingerprint {
  std::string manufacturer;
  std::string model;
  std::string normalized_key;
  std::string device_name;
};

struct ResolvedProfile {
  core::PrinterProfile profile;
  core::PrinterMatchQuality match_quality {core::PrinterMatchQuality::Exact};
};

class ProfileRepository {
 public:
  virtual ~ProfileRepository() = default;

  [[nodiscard]] virtual std::optional<ResolvedProfile> find_profile(
      const PrinterFingerprint& fingerprint) const = 0;

  [[nodiscard]] virtual std::vector<ResolvedProfile> refresh_cached_profiles() = 0;

  virtual void save_learned_profile(const core::PrinterProfile& profile) = 0;
};

}  // namespace duplexprint::cloud
