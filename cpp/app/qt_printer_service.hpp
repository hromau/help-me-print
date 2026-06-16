#pragma once

#include <vector>

#include "types.hpp"

namespace duplexprint::app {

class QtPrinterService {
 public:
  [[nodiscard]] std::vector<core::PrinterInfo> list_printers() const;
};

}  // namespace duplexprint::app
