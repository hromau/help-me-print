#pragma once

#include <QString>

#include "types.hpp"

namespace duplexprint::app {

class QtDocumentService {
 public:
  [[nodiscard]] core::DocumentInfo inspect_pdf(const QString& path) const;
};

}  // namespace duplexprint::app
