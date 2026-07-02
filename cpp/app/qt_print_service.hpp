#pragma once

#include <QString>

class QWidget;

namespace duplexprint::app {

class QtPrintService {
 public:
  [[nodiscard]] bool print_pdf(const QString& pdf_path, const QString& printer_name, QWidget* parent) const;
  void cancel_print_jobs(const QString& printer_name) const;
};

}  // namespace duplexprint::app
