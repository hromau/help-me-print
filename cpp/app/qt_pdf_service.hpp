#pragma once

#include <QString>

#include "types.hpp"

namespace duplexprint::app {

class QtPdfService {
 public:
  [[nodiscard]] QString create_prepared_pass_pdf(
      const core::DocumentInfo& document,
      const core::PassPlan& pass_plan) const;

  [[nodiscard]] QString create_calibration_pdf() const;
};

}  // namespace duplexprint::app
