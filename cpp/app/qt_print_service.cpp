#include "qt_print_service.hpp"

#include <QFileInfo>
#include <QProcess>
#include <QStringList>
#include <stdexcept>

namespace duplexprint::app {

void QtPrintService::print_pdf(const QString& pdf_path, const QString& printer_name) const {
  if (!QFileInfo::exists(pdf_path)) {
    throw std::runtime_error("Prepared PDF does not exist.");
  }

#if defined(Q_OS_WIN)
  const QString program = "powershell";
  const QStringList args {
      "-NoProfile",
      "-ExecutionPolicy",
      "Bypass",
      "-Command",
      QString("Start-Process -FilePath %1 -Verb Print").arg(QString("\"%1\"").arg(pdf_path))
  };
#elif defined(Q_OS_MACOS) || defined(Q_OS_LINUX)
  const QString program = "lp";
  const QStringList args {"-d", printer_name, pdf_path};
#else
  throw std::runtime_error("Printing is not implemented for this platform.");
#endif

  QProcess process;
  process.start(program, args);
  if (!process.waitForStarted() || !process.waitForFinished(30000) || process.exitCode() != 0) {
    const auto error = QString::fromUtf8(process.readAllStandardError()).toStdString();
    throw std::runtime_error(error.empty() ? "Failed to send PDF to printer." : error);
  }
}

}  // namespace duplexprint::app
