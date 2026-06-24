#include "printer_profile_resolver.hpp"

#include "printer_identity.hpp"

namespace duplexprint::platform {

PrinterProfileResolver::PrinterProfileResolver(
    const ProfileStore& local_store,
    const cloud::ProfileRepository& cloud_repository)
    : local_store_(local_store), cloud_repository_(cloud_repository) {}

ResolvedPrinter PrinterProfileResolver::resolve(const core::PrinterInfo& printer) const {
  if (const auto local = local_store_.get(printer.manufacturer, printer.model, printer.name)) {
    auto resolved_printer = printer;
    resolved_printer.training_state = local->source == core::ProfileSource::Cloud
        ? core::PrinterTrainingState::TrainedCloudOverriddenLocal
        : core::PrinterTrainingState::TrainedLocal;
    resolved_printer.resolved_profile_source = core::ProfileSource::Local;
    return ResolvedPrinter {
        .printer = std::move(resolved_printer),
        .profile = *local,
    };
  }

  if (const auto cloud = cloud_repository_.find_profile(PrinterIdentity::fingerprint_for(printer))) {
    auto resolved_printer = printer;
    resolved_printer.training_state = core::PrinterTrainingState::TrainedCloud;
    resolved_printer.resolved_profile_source = core::ProfileSource::Cloud;
    return ResolvedPrinter {
        .printer = std::move(resolved_printer),
        .profile = cloud->profile,
    };
  }

  auto unresolved_printer = printer;
  unresolved_printer.training_state = core::PrinterTrainingState::Untrained;
  unresolved_printer.resolved_profile_source = std::nullopt;
  return ResolvedPrinter {
      .printer = std::move(unresolved_printer),
      .profile = std::nullopt,
  };
}

}  // namespace duplexprint::platform
