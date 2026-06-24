#pragma once

#include <QString>

namespace duplexprint::app {

class QtPrintService {
 public:
  void print_pdf(const QString& pdf_path, const QString& printer_name) const;
};

}  // namespace duplexprint::app
