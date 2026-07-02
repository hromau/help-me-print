#include "qt_pdf_service.hpp"

#include <QDir>
#include <QFont>
#include <QMarginsF>
#include <QPainter>
#include <QPageLayout>
#include <QPageSize>
#include <QPdfDocument>
#include <QPdfWriter>
#include <QStandardPaths>
#include <QUuid>
#include <stdexcept>

namespace duplexprint::app {
namespace {

constexpr int kPdfDpi = 300;

QString temp_pdf_path(const QString& prefix) {
  const auto temp_dir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
  QDir().mkpath(temp_dir);
  return QDir(temp_dir).filePath(prefix + "-" + QUuid::createUuid().toString(QUuid::WithoutBraces) + ".pdf");
}

void configure_writer(QPdfWriter& writer, QPdfDocument& source) {
  const auto first_page = source.pagePointSize(0);
  writer.setPageSize(QPageSize(first_page, QPageSize::Point));
  writer.setPageMargins(QMarginsF(0, 0, 0, 0), QPageLayout::Point);
  writer.setResolution(kPdfDpi);
}

void draw_pdf_page(QPainter& painter, QPdfWriter& writer, QPdfDocument& source, const int zero_based_page) {
  const auto target = writer.pageLayout().paintRectPixels(writer.resolution());
  const auto image = source.render(zero_based_page, target.size());
  if (image.isNull()) {
    throw std::runtime_error("Failed to render PDF page.");
  }
  painter.drawImage(target, image);
}

}  // namespace

QString QtPdfService::create_prepared_pass_pdf(
    const core::DocumentInfo& document,
    const core::PassPlan& pass_plan) const {
  QPdfDocument source;
  if (source.load(QString::fromStdString(document.path)) != QPdfDocument::Error::None) {
    throw std::runtime_error("Failed to open source PDF.");
  }

  const auto output_path = temp_pdf_path(QString("help-me-print-pass-%1").arg(pass_plan.pass));
  QPdfWriter writer(output_path);
  configure_writer(writer, source);

  QPainter painter(&writer);
  bool has_output_page = false;
  for (const auto page_number : pass_plan.pages) {
    if (page_number < 1 || page_number > source.pageCount()) {
      continue;
    }
    if (has_output_page) {
      writer.newPage();
    }
    has_output_page = true;
    draw_pdf_page(painter, writer, source, page_number - 1);
  }

  if (pass_plan.inserts_blank_trailing_sheet) {
    if (has_output_page) {
      writer.newPage();
    }
    painter.fillRect(writer.pageLayout().paintRectPixels(writer.resolution()), Qt::white);
  }

  painter.end();
  return output_path;
}

QString QtPdfService::create_calibration_pdf() const {
  const auto output_path = temp_pdf_path("help-me-print-calibration");
  QPdfWriter writer(output_path);
  writer.setPageSize(QPageSize(QPageSize::A4));
  writer.setPageMargins(QMarginsF(0, 0, 0, 0), QPageLayout::Point);
  writer.setResolution(kPdfDpi);

  QPainter painter(&writer);
  QFont font = painter.font();
  font.setPointSize(220);
  font.setBold(true);
  painter.setFont(font);

  for (int page = 1; page <= 4; ++page) {
    if (page > 1) {
      writer.newPage();
    }
    const auto page_rect = writer.pageLayout().paintRectPixels(writer.resolution());
    painter.fillRect(page_rect, Qt::white);
    painter.drawText(page_rect, Qt::AlignCenter, QString::number(page));
  }

  painter.end();
  return output_path;
}

}  // namespace duplexprint::app
