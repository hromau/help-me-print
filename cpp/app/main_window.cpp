#include "main_window.hpp"

#include <QComboBox>
#include <QDateTime>
#include <QDesktopServices>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSizePolicy>
#include <QUrl>
#include <QStandardPaths>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWidget>
#include <filesystem>
#include <sstream>

#include "formatters.hpp"
#include "printer_identity.hpp"

namespace duplexprint::app {
namespace {

std::filesystem::path profile_store_path() {
  const auto dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
  return std::filesystem::path(dir.toStdString()) / "printer-profiles.json";
}

QString pages_label(const std::vector<std::int32_t>& pages) {
  QStringList values;
  for (const auto page : pages) {
    values.push_back(QString::number(page));
  }
  return values.join(", ");
}

QString pass_summary(const core::PassPlan& pass) {
  auto summary = QString("Pass %1: %2 pages\nPages: %3\nOrder: %4")
      .arg(pass.pass)
      .arg(parity_label(pass.parity))
      .arg(pages_label(pass.pages))
      .arg(order_label(pass.order));
  if (pass.inserts_blank_trailing_sheet) {
    summary += "\nAdds one blank trailing sheet";
  }
  return summary;
}

}  // namespace

MainWindow::MainWindow()
    : profile_store_(profile_store_path()),
      profile_resolver_(profile_store_, cloud_repository_) {
  build_ui();
  refresh_printers();
  update_ui();
}

void MainWindow::build_ui() {
  setWindowTitle("Help Me Print");
  resize(520, 480);

  auto* central = new QWidget(this);
  auto* root = new QVBoxLayout(central);
  root->setContentsMargins(18, 18, 18, 18);
  root->setSpacing(14);

  auto* title = new QLabel("Help Me Print", central);
  QFont title_font = title->font();
  title_font.setPointSize(22);
  title_font.setBold(true);
  title->setFont(title_font);

  document_label_ = new QLabel("No PDF selected", central);
  printer_status_label_ = new QLabel("No printer selected", central);
  status_label_ = new QLabel("Choose a PDF and printer.", central);
  status_label_->setWordWrap(true);

  printer_combo_ = new QComboBox(central);
  top_sheet_input_ = new QLineEdit(central);
  top_sheet_input_->setPlaceholderText("Top sheet page number");

  choose_pdf_button_ = new QPushButton("Choose PDF", central);
  refresh_button_ = new QPushButton("Refresh printers", central);
  train_button_ = new QPushButton("Train printer", central);
  save_training_button_ = new QPushButton("Save training", central);
  first_pass_button_ = new QPushButton("Print first pass", central);
  second_pass_button_ = new QPushButton("Print second pass", central);
  reset_button_ = new QPushButton("Reset job", central);
  support_button_ = new QPushButton("Buy me a coffee", central);
  support_button_->setToolTip("Open buymeacoffee.com/siarheih");

  plan_view_ = new QTextEdit(central);
  plan_view_->setReadOnly(true);
  plan_view_->setFixedHeight(110);
  plan_view_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
  plan_view_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  plan_view_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  plan_view_->setPlaceholderText("Print plan will appear here.");

  auto* header = new QHBoxLayout();
  header->addWidget(title);
  header->addStretch();
  header->addWidget(support_button_);

  auto* form = new QFormLayout();
  form->addRow("Document", document_label_);
  form->addRow("Printer", printer_combo_);
  form->addRow("Printer status", printer_status_label_);
  form->addRow("Calibration result", top_sheet_input_);

  auto* top_actions = new QHBoxLayout();
  top_actions->addWidget(choose_pdf_button_);
  top_actions->addWidget(refresh_button_);

  auto* print_actions = new QHBoxLayout();
  print_actions->addWidget(train_button_);
  print_actions->addWidget(save_training_button_);
  print_actions->addWidget(first_pass_button_);
  print_actions->addWidget(second_pass_button_);

  auto* footer_actions = new QHBoxLayout();
  footer_actions->addWidget(reset_button_);
  footer_actions->addStretch();

  root->addLayout(header);
  root->addLayout(form);
  root->addLayout(top_actions);
  root->addWidget(plan_view_);
  root->addLayout(print_actions);
  root->addLayout(footer_actions);
  root->addWidget(status_label_);

  setCentralWidget(central);

  connect(choose_pdf_button_, &QPushButton::clicked, this, [this] { choose_pdf(); });
  connect(refresh_button_, &QPushButton::clicked, this, [this] { refresh_printers(); });
  connect(train_button_, &QPushButton::clicked, this, [this] { run_calibration(); });
  connect(save_training_button_, &QPushButton::clicked, this, [this] { save_training(); });
  connect(first_pass_button_, &QPushButton::clicked, this, [this] { run_first_pass(); });
  connect(second_pass_button_, &QPushButton::clicked, this, [this] { run_second_pass(); });
  connect(reset_button_, &QPushButton::clicked, this, [this] { reset_job(); });
  connect(support_button_, &QPushButton::clicked, this, [this] { open_support_link(); });
  connect(printer_combo_, &QComboBox::currentIndexChanged, this, [this] { update_ui(); });
}

void MainWindow::refresh_printers() {
  try {
    printers_.clear();
    for (const auto& printer : printer_service_.list_printers()) {
      printers_.push_back(profile_resolver_.resolve(printer));
    }

    printer_combo_->clear();
    for (const auto& resolved : printers_) {
      printer_combo_->addItem(
          to_qstring(resolved.printer.name) + " - " + training_state_label(resolved.printer.training_state));
    }
    set_status(printers_.empty() ? "No system printers found." : "Printers refreshed.");
  } catch (const std::exception& error) {
    QMessageBox::critical(this, "Printer discovery failed", error.what());
  }
  update_ui();
}

void MainWindow::choose_pdf() {
  const auto path = QFileDialog::getOpenFileName(this, "Choose PDF", QString(), "PDF files (*.pdf)");
  if (path.isEmpty()) {
    return;
  }

  try {
    document_ = document_service_.inspect_pdf(path);
    workflow_.reset();
    second_pass_plan_.reset();
    stage_ = Stage::Setup;
    set_status("PDF loaded.");
  } catch (const std::exception& error) {
    QMessageBox::critical(this, "PDF error", error.what());
  }
  update_ui();
}

std::optional<platform::ResolvedPrinter> MainWindow::selected_printer() const {
  const auto index = printer_combo_->currentIndex();
  if (index < 0 || static_cast<std::size_t>(index) >= printers_.size()) {
    return std::nullopt;
  }
  return printers_[static_cast<std::size_t>(index)];
}

void MainWindow::run_calibration() {
  const auto printer = selected_printer();
  if (!printer) {
    set_status("Choose a printer first.");
    return;
  }

  try {
    const auto calibration_pdf = pdf_service_.create_calibration_pdf();
    const auto calibration_document = document_service_.inspect_pdf(calibration_pdf);
    calibration_workflow_ = workflow_engine_.create_calibration_workflow(calibration_document, printer->printer);
    const auto prepared = pdf_service_.create_prepared_pass_pdf(calibration_workflow_->document, calibration_workflow_->first_pass);
    print_service_.print_pdf(prepared, to_qstring(printer->printer.device_name));
    stage_ = Stage::AwaitingTraining;
    set_status("Calibration pages sent to printer. Enter the page number visible on top of the printed stack.");
  } catch (const std::exception& error) {
    QMessageBox::critical(this, "Calibration failed", error.what());
  }
  update_ui();
}

core::PrinterProfile MainWindow::make_profile_from_calibration(const core::PrinterInfo& printer) const {
  const auto learned = workflow_engine_.learn_from_calibration(top_sheet_input_->text().toStdString());
  return core::PrinterProfile {
      .printer_name = printer.name,
      .manufacturer = printer.manufacturer,
      .model = printer.model,
      .normalized_key = platform::PrinterIdentity::normalize_key(printer.manufacturer, printer.model),
      .output_face = learned.output_face,
      .first_pass_parity = learned.first_pass_parity,
      .second_pass_parity = learned.second_pass_parity,
      .first_pass_order = learned.first_pass_order,
      .even_pages_order = learned.even_pages_order,
      .reload_method = learned.reload_method,
      .confidence = 80,
      .learned_at = QDateTime::currentDateTimeUtc().toString(Qt::ISODate).toStdString(),
      .source = core::ProfileSource::Local,
      .schema_version = 1,
  };
}

void MainWindow::save_training() {
  const auto printer = selected_printer();
  if (!printer || top_sheet_input_->text().trimmed().isEmpty()) {
    set_status("Enter the top sheet page number first.");
    return;
  }

  try {
    const auto profile = make_profile_from_calibration(printer->printer);
    profile_store_.save(profile);
    auto status = QString("Printer learned locally. Future jobs can print immediately.");
    try {
      cloud_repository_.save_learned_profile(profile);
      status = "Printer learned locally and submitted to Easure. Future jobs can print immediately.";
    } catch (const std::exception&) {
      status = "Printer learned locally. Easure submission failed, but future local jobs can print immediately.";
    }
    calibration_workflow_.reset();
    top_sheet_input_->clear();
    stage_ = Stage::Setup;
    refresh_printers();
    set_status(status);
  } catch (const std::exception& error) {
    QMessageBox::critical(this, "Training failed", error.what());
  }
  update_ui();
}

void MainWindow::run_first_pass() {
  const auto printer = selected_printer();
  if (!document_ || !printer) {
    set_status("Choose both a PDF and printer.");
    return;
  }
  if (!printer->profile) {
    set_status("Train this printer first, or wait until an Easure cloud profile exists.");
    return;
  }

  try {
    workflow_ = workflow_engine_.create_workflow(*document_, printer->printer, printer->profile);
    const auto prepared = pdf_service_.create_prepared_pass_pdf(workflow_->document, workflow_->first_pass);
    print_service_.print_pdf(prepared, to_qstring(workflow_->printer.device_name));
    stage_ = Stage::AwaitingSecondPass;
    set_status("First pass sent to printer. Reload the stack, then print the second pass.");
  } catch (const std::exception& error) {
    QMessageBox::critical(this, "First pass failed", error.what());
  }
  update_ui();
}

void MainWindow::run_second_pass() {
  if (!workflow_) {
    return;
  }

  try {
    second_pass_plan_ = workflow_->second_pass;
    const auto prepared = pdf_service_.create_prepared_pass_pdf(workflow_->document, workflow_->second_pass);
    print_service_.print_pdf(prepared, to_qstring(workflow_->printer.device_name));
    stage_ = Stage::Done;
    set_status("Second pass sent to printer.");
  } catch (const std::exception& error) {
    QMessageBox::critical(this, "Second pass failed", error.what());
  }
  update_ui();
}

void MainWindow::reset_job() {
  workflow_.reset();
  calibration_workflow_.reset();
  second_pass_plan_.reset();
  top_sheet_input_->clear();
  stage_ = Stage::Setup;
  set_status("Job reset.");
  update_ui();
}

void MainWindow::open_support_link() {
  if (!QDesktopServices::openUrl(QUrl("https://buymeacoffee.com/siarheih"))) {
    QMessageBox::warning(this,
                         "Unable to open link",
                         "Could not open https://buymeacoffee.com/siarheih");
  }
}

void MainWindow::set_status(const QString& message) {
  status_label_->setText(message);
}

void MainWindow::update_ui() {
  document_label_->setText(document_
      ? QString("%1 (%2 pages)").arg(to_qstring(document_->file_name)).arg(document_->page_count)
      : "No PDF selected");

  const auto printer = selected_printer();
  if (printer) {
    auto status = training_state_label(printer->printer.training_state);
    if (printer->printer.resolved_profile_source) {
      status += " from " + source_label(*printer->printer.resolved_profile_source);
    }
    printer_status_label_->setText(status);
  } else {
    printer_status_label_->setText("No printer selected");
  }

  QStringList plan_lines;
  if (workflow_) {
    plan_lines << pass_summary(workflow_->first_pass);
    plan_lines << "";
    plan_lines << pass_summary(workflow_->second_pass);
  } else if (calibration_workflow_) {
    plan_lines << pass_summary(calibration_workflow_->first_pass);
  } else if (printer && printer->profile) {
    plan_lines << "This printer is already known. You can print immediately.";
  } else {
    plan_lines << "No print plan yet.";
  }
  plan_view_->setPlainText(plan_lines.join("\n"));

  const bool has_printer = printer.has_value();
  const bool has_profile = printer && printer->profile.has_value();
  choose_pdf_button_->setEnabled(true);
  refresh_button_->setEnabled(true);
  train_button_->setEnabled(has_printer && stage_ == Stage::Setup);
  save_training_button_->setEnabled(stage_ == Stage::AwaitingTraining);
  first_pass_button_->setEnabled(document_.has_value() && has_profile && stage_ == Stage::Setup);
  second_pass_button_->setEnabled(workflow_.has_value() && stage_ == Stage::AwaitingSecondPass);
  top_sheet_input_->setEnabled(stage_ == Stage::AwaitingTraining);
  reset_button_->setEnabled(stage_ != Stage::Setup || workflow_.has_value() || calibration_workflow_.has_value());
}

}  // namespace duplexprint::app
