#include "qt_printer_service.hpp"

#include <QProcess>
#include <QRegularExpression>
#include <QStringList>

namespace duplexprint::app {
namespace {

std::pair<std::string, std::string> split_make_model(const QString& name) {
  auto normalized_name = name;
  normalized_name.replace(QRegularExpression("[_\\-]+"), " ");
  normalized_name = normalized_name.simplified();

  const auto parts = normalized_name.split(' ', Qt::SkipEmptyParts);
  if (parts.isEmpty()) {
    return {"Unknown", name.toStdString()};
  }

  const auto manufacturer = parts.first().toStdString();
  QStringList model_parts = parts;
  model_parts.removeFirst();
  const auto model = model_parts.isEmpty() ? normalized_name.toStdString() : model_parts.join(' ').toStdString();
  return {manufacturer, model};
}

}  // namespace

std::vector<core::PrinterInfo> QtPrinterService::list_printers() const {
  std::vector<core::PrinterInfo> printers;

#if defined(Q_OS_WIN)
  const QString program = "powershell";
  const QStringList args {"-NoProfile", "-Command", "Get-Printer | Select-Object -ExpandProperty Name"};
#elif defined(Q_OS_MACOS) || defined(Q_OS_LINUX)
  const QString program = "lpstat";
  const QStringList args {"-e"};
#else
  return printers;
#endif

  QProcess process;
  process.start(program, args);
  if (!process.waitForFinished(5000) || process.exitCode() != 0) {
    return printers;
  }

  const auto lines = QString::fromUtf8(process.readAllStandardOutput()).split('\n', Qt::SkipEmptyParts);
  for (const auto& line : lines) {
    const auto name = line.simplified();
    if (name.isEmpty()) {
      continue;
    }
    const auto [manufacturer, model] = split_make_model(name);
    printers.push_back(core::PrinterInfo {
        .id = name.toStdString(),
        .name = name.toStdString(),
        .device_name = name.toStdString(),
        .manufacturer = manufacturer,
        .model = model,
    });
  }
  return printers;
}

}  // namespace duplexprint::app
