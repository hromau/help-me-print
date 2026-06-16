#include "qt_document_service.hpp"

#include <QFileInfo>
#include <QPdfDocument>
#include <stdexcept>

namespace duplexprint::app {

core::DocumentInfo QtDocumentService::inspect_pdf(const QString& path) const {
  QPdfDocument pdf;
  const auto status = pdf.load(path);
  if (status != QPdfDocument::Error::None) {
    throw std::runtime_error("Failed to load PDF document.");
  }

  return core::DocumentInfo {
      .path = path.toStdString(),
      .file_name = QFileInfo(path).fileName().toStdString(),
      .page_count = pdf.pageCount(),
  };
}

}  // namespace duplexprint::app
