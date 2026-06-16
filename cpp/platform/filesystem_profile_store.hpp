#pragma once

#include <filesystem>

#include "profile_store.hpp"

namespace duplexprint::platform {

class FilesystemProfileStore : public ProfileStore {
 public:
  explicit FilesystemProfileStore(std::filesystem::path file_path);

  [[nodiscard]] std::vector<core::PrinterProfile> list() const override;
  [[nodiscard]] std::optional<core::PrinterProfile> get(
      const std::string& manufacturer,
      const std::string& model,
      const std::string& printer_name) const override;
  void save(const core::PrinterProfile& profile) override;

 private:
  [[nodiscard]] std::vector<core::PrinterProfile> read_profiles() const;
  void write_profiles(const std::vector<core::PrinterProfile>& profiles) const;

  std::filesystem::path file_path_;
};

}  // namespace duplexprint::platform
