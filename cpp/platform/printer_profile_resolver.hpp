#pragma once

#include <optional>

#include "profile_repository.hpp"
#include "profile_store.hpp"
#include "types.hpp"

namespace duplexprint::platform {

struct ResolvedPrinter {
  core::PrinterInfo printer;
  std::optional<core::PrinterProfile> profile;
};

class PrinterProfileResolver {
 public:
  PrinterProfileResolver(const ProfileStore& local_store, const cloud::ProfileRepository& cloud_repository);

  [[nodiscard]] ResolvedPrinter resolve(const core::PrinterInfo& printer) const;

 private:
  const ProfileStore& local_store_;
  const cloud::ProfileRepository& cloud_repository_;
};

}  // namespace duplexprint::platform
