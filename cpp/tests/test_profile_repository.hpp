#pragma once

#include <optional>
#include <vector>

#include "profile_repository.hpp"

namespace duplexprint::tests {

class TestProfileRepository : public cloud::ProfileRepository {
 public:
  explicit TestProfileRepository(std::vector<cloud::ResolvedProfile> profiles = {})
      : profiles_(std::move(profiles)) {}

  [[nodiscard]] std::optional<cloud::ResolvedProfile> find_profile(
      const cloud::PrinterFingerprint& fingerprint) const override {
    for (const auto& resolved : profiles_) {
      const auto key = resolved.profile.manufacturer + ":" + resolved.profile.model;
      if (key == fingerprint.manufacturer + ":" + fingerprint.model ||
          key == fingerprint.normalized_key) {
        return resolved;
      }
    }
    return std::nullopt;
  }

  [[nodiscard]] std::vector<cloud::ResolvedProfile> refresh_cached_profiles() override {
    return profiles_;
  }

  void save_learned_profile(const core::PrinterProfile& profile) override {
    profiles_.push_back(cloud::ResolvedProfile {
        .profile = profile,
        .match_quality = core::PrinterMatchQuality::Exact,
    });
  }

 private:
  std::vector<cloud::ResolvedProfile> profiles_;
};

}  // namespace duplexprint::tests
