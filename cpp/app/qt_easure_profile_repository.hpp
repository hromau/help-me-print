#pragma once

#include <QUrl>

#include "profile_repository.hpp"

namespace duplexprint::app {

class QtEasureProfileRepository : public cloud::ProfileRepository {
 public:
  explicit QtEasureProfileRepository(QUrl base_url = QUrl("https://easure-duplexprint-api.azurewebsites.net/api"));

  [[nodiscard]] std::optional<cloud::ResolvedProfile> find_profile(
      const cloud::PrinterFingerprint& fingerprint) const override;

  [[nodiscard]] std::vector<cloud::ResolvedProfile> refresh_cached_profiles() override;

  void save_learned_profile(const core::PrinterProfile& profile) override;

 private:
  QUrl base_url_;
};

}  // namespace duplexprint::app
