#pragma once

#include <QMainWindow>

#include <optional>
#include <vector>

#include "filesystem_profile_store.hpp"
#include "printer_profile_resolver.hpp"
#include "qt_document_service.hpp"
#include "qt_easure_profile_repository.hpp"
#include "qt_pdf_service.hpp"
#include "qt_print_service.hpp"
#include "qt_printer_service.hpp"
#include "workflow_engine.hpp"

QT_BEGIN_NAMESPACE
class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QTextEdit;
QT_END_NAMESPACE

namespace duplexprint::app {

class MainWindow : public QMainWindow {
 public:
  MainWindow();

 private:
  enum class Stage {
    Setup,
    AwaitingTraining,
    AwaitingSecondPass,
    Done
  };

  void build_ui();
  void refresh_printers();
  void choose_pdf();
  void run_calibration();
  void save_training();
  void run_first_pass();
  void run_second_pass();
  void reset_job();
  void open_support_link();
  void update_ui();
  void set_status(const QString& message);

  [[nodiscard]] std::optional<platform::ResolvedPrinter> selected_printer() const;
  [[nodiscard]] core::PrinterProfile make_profile_from_calibration(const core::PrinterInfo& printer) const;

  QLabel* document_label_ {nullptr};
  QLabel* printer_status_label_ {nullptr};
  QLabel* status_label_ {nullptr};
  QComboBox* printer_combo_ {nullptr};
  QLineEdit* top_sheet_input_ {nullptr};
  QTextEdit* plan_view_ {nullptr};
  QPushButton* choose_pdf_button_ {nullptr};
  QPushButton* refresh_button_ {nullptr};
  QPushButton* train_button_ {nullptr};
  QPushButton* save_training_button_ {nullptr};
  QPushButton* first_pass_button_ {nullptr};
  QPushButton* second_pass_button_ {nullptr};
  QPushButton* reset_button_ {nullptr};
  QPushButton* support_button_ {nullptr};

  core::WorkflowEngine workflow_engine_;
  QtDocumentService document_service_;
  QtPrinterService printer_service_;
  QtPdfService pdf_service_;
  QtPrintService print_service_;
  QtEasureProfileRepository cloud_repository_;
  platform::FilesystemProfileStore profile_store_;
  platform::PrinterProfileResolver profile_resolver_;

  std::vector<platform::ResolvedPrinter> printers_;
  std::optional<core::DocumentInfo> document_;
  std::optional<core::WorkflowSummary> workflow_;
  std::optional<core::WorkflowSummary> calibration_workflow_;
  std::optional<core::PassPlan> second_pass_plan_;
  Stage stage_ {Stage::Setup};
};

}  // namespace duplexprint::app
