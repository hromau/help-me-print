#include "qt_print_service.hpp"

#include <QDialog>
#include <QFileInfo>
#include <QPainter>
#include <QPdfDocument>
#include <QProcess>
#include <QPrintDialog>
#include <QPrinter>
#include <QRect>
#include <QRectF>
#include <QStringList>
#include <stdexcept>

namespace duplexprint::app {
namespace {

void run_process_or_throw(const QString& program, const QStringList& args, const char* fallback_error) {
  QProcess process;
  process.start(program, args);
  if (!process.waitForStarted() || !process.waitForFinished(30000) || process.exitCode() != 0) {
    const auto error = QString::fromUtf8(process.readAllStandardError()).toStdString();
    throw std::runtime_error(error.empty() ? fallback_error : error);
  }
}

QRect fit_rect(const QSize& source, const QRectF& target) {
  if (source.isEmpty() || target.isEmpty()) {
    return target.toAlignedRect();
  }

  QSize fitted = source;
  fitted.scale(target.size().toSize(), Qt::KeepAspectRatio);
  QRect rect(QPoint(0, 0), fitted);
  rect.moveCenter(target.center().toPoint());
  return rect;
}

}  // namespace

bool QtPrintService::print_pdf(const QString& pdf_path, const QString& printer_name, QWidget* parent) const {
  if (!QFileInfo::exists(pdf_path)) {
    throw std::runtime_error("Prepared PDF does not exist.");
  }

  QPdfDocument source;
  if (source.load(pdf_path) != QPdfDocument::Error::None) {
    throw std::runtime_error("Failed to open prepared PDF.");
  }
  if (source.pageCount() == 0) {
    throw std::runtime_error("Prepared PDF has no pages.");
  }

  QPrinter printer(QPrinter::HighResolution);
  if (!printer_name.trimmed().isEmpty()) {
    printer.setPrinterName(printer_name);
  }
  printer.setDocName(QFileInfo(pdf_path).completeBaseName());

  QPrintDialog dialog(&printer, parent);
  dialog.setWindowTitle("Print");
  if (dialog.exec() != QDialog::Accepted) {
    return false;
  }

  QPainter painter(&printer);
  if (!painter.isActive()) {
    throw std::runtime_error("Failed to start printer painter.");
  }

  for (int page = 0; page < source.pageCount(); ++page) {
    if (page > 0 && !printer.newPage()) {
      throw std::runtime_error("Failed to create printer page.");
    }

    const auto page_rect = printer.pageRect(QPrinter::DevicePixel).toAlignedRect();
    const auto source_points = source.pagePointSize(page).toSize();
    const auto target_rect = fit_rect(source_points, page_rect);
    const auto image = source.render(page, target_rect.size());
    if (image.isNull()) {
      throw std::runtime_error("Failed to render PDF page for printing.");
    }
    painter.drawImage(target_rect, image);
  }

  painter.end();
  return true;
}

void QtPrintService::cancel_print_jobs(const QString& printer_name) const {
  if (printer_name.trimmed().isEmpty()) {
    throw std::runtime_error("No printer selected.");
  }

#if defined(Q_OS_WIN)
  const QString program = "powershell";
  const QStringList args {
      "-NoProfile",
      "-ExecutionPolicy",
      "Bypass",
      "-Command",
      "& { param($PrinterName) Get-PrintJob -PrinterName $PrinterName | Remove-PrintJob }",
      printer_name
  };
#elif defined(Q_OS_MACOS) || defined(Q_OS_LINUX)
  const QString program = "cancel";
  const QStringList args {"-a", printer_name};
#else
  throw std::runtime_error("Print cancellation is not implemented for this platform.");
#endif

  run_process_or_throw(program, args, "Failed to cancel print jobs.");
}

}  // namespace duplexprint::app
