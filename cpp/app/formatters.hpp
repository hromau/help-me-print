#pragma once

#include <QString>

#include "types.hpp"

namespace duplexprint::app {

inline QString to_qstring(const std::string& value) {
  return QString::fromStdString(value);
}

inline QString parity_label(const core::PageParity parity) {
  return parity == core::PageParity::Even ? "even" : "odd";
}

inline QString order_label(const core::PageOrder order) {
  return order == core::PageOrder::Reverse ? "reverse" : "normal";
}

inline QString source_label(const core::ProfileSource source) {
  switch (source) {
    case core::ProfileSource::Local:
      return "local";
    case core::ProfileSource::Cloud:
      return "Easure cloud";
    case core::ProfileSource::Manual:
      return "manual";
  }
  return "unknown";
}

inline QString training_state_label(const core::PrinterTrainingState state) {
  switch (state) {
    case core::PrinterTrainingState::Untrained:
      return "Needs training";
    case core::PrinterTrainingState::TrainedLocal:
      return "Already trained";
    case core::PrinterTrainingState::TrainedCloud:
      return "Cloud profile available";
    case core::PrinterTrainingState::TrainedCloudOverriddenLocal:
      return "Local override";
  }
  return "Unknown";
}

}  // namespace duplexprint::app
