#include "main_window.hpp"

#include <QComboBox>
#include <QDateTime>
#include <QDesktopServices>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QShowEvent>
#include <QSizePolicy>
#include <QStandardPaths>
#include <QStyle>
#include <QToolButton>
#include <QUrl>
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
  refresh_printers(false);
  update_ui();
}

void MainWindow::build_ui() {
  setWindowTitle("Help Me Print");

  auto* central = new QWidget(this);
  central->setObjectName("root");
  central->setStyleSheet(
      "#root { background-color: #08111d; }"
      "QFrame[card='true'] {"
      "  background-color: #0d1828;"
      "  border: 1px solid #15283f;"
      "  border-radius: 16px;"
      "}"
      "QLabel { color: #f3f7ff; }"
      "QLabel[muted='true'] { color: #97a7bf; }"
      "QLabel[status='good'] { color: #d8e6ff; }"
      "QPushButton[variant='primary'] {"
      "  background-color: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #2b6cff, stop:1 #2563eb);"
      "  color: white;"
      "  border: 1px solid #4d83ff;"
      "  border-radius: 14px;"
      "  font-weight: 700;"
      "  padding: 10px 16px;"
      "}"
      "QPushButton[variant='primary']:disabled {"
      "  background-color: #233247;"
      "  color: #71839e;"
      "  border-color: #233247;"
      "}"
      "QPushButton[variant='secondary'] {"
      "  background-color: #101d31;"
      "  color: #eef4ff;"
      "  border: 1px solid #223655;"
      "  border-radius: 14px;"
      "  font-weight: 600;"
      "  padding: 8px 14px;"
      "}"
      "QToolButton {"
      "  background-color: #101d31;"
      "  border: 1px solid #223655;"
      "  border-radius: 12px;"
      "  color: #8ab2ff;"
      "}"
      "QComboBox {"
      "  background-color: #101d31;"
      "  color: #f6f8fc;"
      "  border: 1px solid #2a4165;"
      "  border-radius: 12px;"
      "  padding: 10px 40px 10px 14px;"
      "  font-size: 15px;"
      "}"
      "QComboBox::drop-down {"
      "  border: none;"
      "  width: 40px;"
      "}"
      "QComboBox QAbstractItemView {"
      "  background-color: #101d31;"
      "  color: #f6f8fc;"
      "  border: 1px solid #2a4165;"
      "  selection-background-color: #2563eb;"
      "}"
      "QLineEdit {"
      "  background-color: #101d31;"
      "  color: #f6f8fc;"
      "  border: 1px solid #2a4165;"
      "  border-radius: 14px;"
      "  padding: 12px 14px;"
      "}");
  auto* root = new QVBoxLayout(central);
  root->setContentsMargins(14, 8, 14, 12);
  root->setSpacing(8);
  root->setSizeConstraint(QLayout::SetFixedSize);

  document_label_ = new QLabel("No PDF selected", central);
  document_label_->setProperty("muted", true);
  printer_status_label_ = new QLabel("No printer selected", central);
  printer_status_label_->setProperty("status", "good");
  status_label_ = new QLabel("", central);
  status_label_->setWordWrap(true);
  status_label_->setProperty("muted", true);
  status_label_->hide();

  printer_combo_ = new QComboBox(central);
  printer_combo_->setMinimumHeight(44);

  choose_pdf_button_ = new QPushButton("Choose PDF", central);
  refresh_button_ = new QToolButton(central);
  train_button_ = new QPushButton("Train printer", central);
  save_training_button_ = new QPushButton("Finish training", central);
  first_pass_button_ = new QPushButton("Print first pass", central);
  second_pass_button_ = new QPushButton("Print second pass", central);
  reset_button_ = new QPushButton("Start over", central);
  support_button_ = new QPushButton("Buy me a coffee", central);
  support_button_->setToolTip("Open buymeacoffee.com/siarheih");

  choose_pdf_button_->setProperty("variant", "primary");
  train_button_->setProperty("variant", "primary");
  save_training_button_->setProperty("variant", "primary");
  first_pass_button_->setProperty("variant", "primary");
  second_pass_button_->setProperty("variant", "primary");
  support_button_->setProperty("variant", "secondary");
  reset_button_->setProperty("variant", "secondary");

  choose_pdf_button_->setIcon(style()->standardIcon(QStyle::SP_DialogOpenButton));
  train_button_->setIcon(style()->standardIcon(QStyle::SP_ComputerIcon));
  save_training_button_->setIcon(style()->standardIcon(QStyle::SP_DialogApplyButton));
  first_pass_button_->setIcon(style()->standardIcon(QStyle::SP_FileDialogDetailedView));
  second_pass_button_->setIcon(style()->standardIcon(QStyle::SP_ArrowForward));

  refresh_button_->setIcon(style()->standardIcon(QStyle::SP_BrowserReload));
  refresh_button_->setToolTip("Refresh printers");
  refresh_button_->setAutoRaise(false);
  refresh_button_->setFixedSize(44, 40);
  refresh_button_->setIconSize(QSize(18, 18));

  const auto configure_action_button = [](QPushButton* button) {
    button->setMinimumHeight(46);
    button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    QFont font = button->font();
    font.setPointSize(12);
    font.setBold(true);
    button->setFont(font);
  };
  configure_action_button(train_button_);
  configure_action_button(save_training_button_);
  configure_action_button(first_pass_button_);
  configure_action_button(second_pass_button_);
  choose_pdf_button_->setMinimumHeight(40);
  choose_pdf_button_->setMinimumWidth(154);
  QFont choose_font = choose_pdf_button_->font();
  choose_font.setPointSize(12);
  choose_font.setBold(true);
  choose_pdf_button_->setFont(choose_font);

  reset_button_->setMinimumHeight(32);
  reset_button_->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
  support_button_->setMinimumHeight(40);
  support_button_->setMinimumWidth(154);
  support_button_->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);

  auto make_icon_label = [this](QStyle::StandardPixmap icon, const QSize& size) {
    auto* label = new QLabel(this);
    label->setPixmap(style()->standardIcon(icon).pixmap(size));
    label->setFixedSize(size);
    label->setAlignment(Qt::AlignCenter);
    return label;
  };

  auto* document_card = new QFrame(central);
  document_card->setProperty("card", true);
  document_card->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
  auto* document_layout = new QHBoxLayout(document_card);
  document_layout->setContentsMargins(14, 12, 14, 12);
  document_layout->setSpacing(10);

  auto* document_icon = make_icon_label(QStyle::SP_FileIcon, QSize(22, 22));
  auto* document_title = new QLabel("Document", document_card);
  QFont section_font = document_title->font();
  section_font.setPointSize(14);
  section_font.setBold(true);
  document_title->setFont(section_font);
  QFont value_font = document_label_->font();
  value_font.setPointSize(13);
  document_label_->setFont(value_font);

  document_layout->addWidget(document_icon);
  document_layout->addWidget(document_title);
  document_layout->addWidget(document_label_, 1);
  document_layout->addWidget(support_button_);
  document_layout->addWidget(choose_pdf_button_);

  auto* printer_card = new QFrame(central);
  printer_card->setProperty("card", true);
  printer_card->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
  auto* printer_card_layout = new QVBoxLayout(printer_card);
  printer_card_layout->setContentsMargins(14, 14, 14, 14);
  printer_card_layout->setSpacing(10);

  auto* printer_header = new QHBoxLayout();
  printer_header->setSpacing(10);
  auto* printer_icon = make_icon_label(QStyle::SP_ComputerIcon, QSize(22, 22));
  auto* printer_title = new QLabel("Printer", printer_card);
  printer_title->setFont(section_font);
  printer_header->addWidget(printer_icon);
  printer_header->addWidget(printer_title);
  printer_header->addStretch();

  auto* printer_row = new QHBoxLayout();
  printer_row->setSpacing(10);
  printer_row->addWidget(printer_combo_, 1);
  printer_row->addWidget(refresh_button_);

  auto* printer_status_row = new QHBoxLayout();
  printer_status_row->setSpacing(8);
  QFont status_font = printer_status_label_->font();
  status_font.setPointSize(12);
  printer_status_label_->setFont(status_font);
  printer_status_label_->setProperty("muted", true);
  printer_status_row->addWidget(printer_status_label_);
  printer_status_row->addStretch();

  printer_card_layout->addLayout(printer_header);
  printer_card_layout->addLayout(printer_row);
  printer_card_layout->addLayout(printer_status_row);

  action_card_ = new QFrame(central);
  action_card_->setFrameShape(QFrame::NoFrame);
  action_card_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
  auto* action_layout = new QVBoxLayout(action_card_);
  action_layout->setContentsMargins(0, 0, 0, 0);
  action_layout->setSpacing(6);
  action_layout->addWidget(train_button_);
  action_layout->addWidget(save_training_button_);
  action_layout->addWidget(first_pass_button_);
  action_layout->addWidget(second_pass_button_);

  footer_container_ = new QWidget(central);
  footer_container_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
  auto* footer = new QHBoxLayout(footer_container_);
  footer->setContentsMargins(0, 0, 0, 0);
  footer->setSpacing(8);
  footer->addWidget(reset_button_);
  footer->addWidget(status_label_, 1);

  root->addWidget(document_card);
  root->addWidget(printer_card);
  root->addWidget(action_card_);
  root->addWidget(footer_container_);

  setCentralWidget(central);
  sync_window_size();

  connect(choose_pdf_button_, &QPushButton::clicked, this, [this] { choose_pdf(); });
  connect(refresh_button_, &QToolButton::clicked, this, [this] { refresh_printers(true); });
  connect(train_button_, &QPushButton::clicked, this, [this] { run_calibration(); });
  connect(save_training_button_, &QPushButton::clicked, this, [this] { save_training(); });
  connect(first_pass_button_, &QPushButton::clicked, this, [this] { run_first_pass(); });
  connect(second_pass_button_, &QPushButton::clicked, this, [this] { run_second_pass(); });
  connect(reset_button_, &QPushButton::clicked, this, [this] { reset_job(); });
  connect(support_button_, &QPushButton::clicked, this, [this] { open_support_link(); });
  connect(printer_combo_, &QComboBox::currentIndexChanged, this, [this] { update_ui(); });
}

void MainWindow::sync_window_size() {
  if (centralWidget() == nullptr) {
    return;
  }

  centralWidget()->adjustSize();
  adjustSize();
  setFixedSize(sizeHint());
}

void MainWindow::showEvent(QShowEvent* event) {
  QMainWindow::showEvent(event);
  sync_window_size();
}

void MainWindow::refresh_printers(const bool announce) {
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
    if (announce) {
      set_status(printers_.empty() ? "No system printers found." : "Printer list updated.");
    }
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
    set_status("");
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
    if (!print_service_.print_pdf(prepared, to_qstring(printer->printer.device_name), this)) {
      set_status("Printing cancelled.");
      update_ui();
      return;
    }
    stage_ = Stage::AwaitingTraining;
    set_status("Calibration pages sent to printer. Finish training in the dialog after checking the printed stack.");
  } catch (const std::exception& error) {
    QMessageBox::critical(this, "Calibration failed", error.what());
  }
  update_ui();
  if (stage_ == Stage::AwaitingTraining) {
    prompt_for_calibration_result();
  }
}

core::PrinterProfile MainWindow::make_profile_from_calibration(
    const core::PrinterInfo& printer,
    const QString& observed_top_page) const {
  const auto learned = workflow_engine_.learn_from_calibration(observed_top_page.trimmed().toStdString());
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
  if (stage_ != Stage::AwaitingTraining) {
    set_status("Training is not waiting for a calibration result.");
    return;
  }
  prompt_for_calibration_result();
}

void MainWindow::complete_training(const QString& observed_top_page) {
  const auto printer = selected_printer();
  if (!printer || observed_top_page.trimmed().isEmpty()) {
    set_status("Enter the top sheet page number first.");
    return;
  }
  try {
    const auto profile = make_profile_from_calibration(printer->printer, observed_top_page);
    profile_store_.save(profile);
    auto status = QString("Printer learned locally. Future jobs can print immediately.");
    try {
      cloud_repository_.save_learned_profile(profile);
      status = "Printer learned locally and submitted to Easure. Future jobs can print immediately.";
    } catch (const std::exception&) {
      status = "Printer learned locally. Easure submission failed, but future local jobs can print immediately.";
    }
    calibration_workflow_.reset();
    stage_ = Stage::Setup;
    refresh_printers(false);
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
    if (!print_service_.print_pdf(prepared, to_qstring(workflow_->printer.device_name), this)) {
      set_status("Printing cancelled.");
      update_ui();
      return;
    }
    stage_ = Stage::AwaitingSecondPass;
    set_status("First pass sent to printer. Reload the stack, then continue with the second pass.");
  } catch (const std::exception& error) {
    QMessageBox::critical(this, "First pass failed", error.what());
  }
  update_ui();
  if (stage_ == Stage::AwaitingSecondPass) {
    prompt_for_second_pass();
  }
}

void MainWindow::run_second_pass() {
  if (!workflow_) {
    return;
  }

  try {
    second_pass_plan_ = workflow_->second_pass;
    const auto prepared = pdf_service_.create_prepared_pass_pdf(workflow_->document, workflow_->second_pass);
    if (!print_service_.print_pdf(prepared, to_qstring(workflow_->printer.device_name), this)) {
      set_status("Printing cancelled.");
      update_ui();
      return;
    }
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
  stage_ = Stage::Setup;
  set_status("Job reset.");
  update_ui();
}

void MainWindow::prompt_for_calibration_result() {
  if (stage_ != Stage::AwaitingTraining) {
    return;
  }

  QDialog dialog(this);
  dialog.setWindowTitle("Finish training");
  dialog.setModal(true);

  auto* layout = new QVBoxLayout(&dialog);
  auto* title = new QLabel("Training step 2 of 2", &dialog);
  QFont title_font = title->font();
  title_font.setBold(true);
  title->setFont(title_font);

  auto* body = new QLabel(
      "Look at the printed calibration stack and enter the page number on the top sheet. "
      "This is saved as the printer's duplex profile.",
      &dialog);
  body->setWordWrap(true);

  auto* input = new QLineEdit(&dialog);
  input->setPlaceholderText("Top sheet page number");
  input->setFocus();

  auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
  buttons->button(QDialogButtonBox::Ok)->setText("Save training");

  layout->addWidget(title);
  layout->addWidget(body);
  layout->addWidget(input);
  layout->addWidget(buttons);

  connect(buttons, &QDialogButtonBox::accepted, &dialog, [&dialog, input, this] {
    if (input->text().trimmed().isEmpty()) {
      QMessageBox::warning(&dialog, "Missing page number", "Enter the page number that is visible on top.");
      return;
    }
    complete_training(input->text());
    if (stage_ == Stage::Setup) {
      dialog.accept();
    }
  });
  connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

  dialog.exec();
}

void MainWindow::prompt_for_second_pass() {
  if (stage_ != Stage::AwaitingSecondPass) {
    return;
  }

  QMessageBox dialog(this);
  dialog.setWindowTitle("Reload and continue");
  dialog.setIcon(QMessageBox::Information);
  dialog.setText("First pass is done.");
  dialog.setInformativeText(
      "Reload the printed stack into the tray, then continue with the second pass.");
  auto* continue_button = dialog.addButton("Print second pass now", QMessageBox::AcceptRole);
  dialog.exec();

  if (dialog.clickedButton() == continue_button) {
    run_second_pass();
  }
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
  status_label_->setVisible(!message.trimmed().isEmpty());
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

  const bool has_printer = printer.has_value();
  const bool has_profile = printer && printer->profile.has_value();
  const bool awaiting_training = stage_ == Stage::AwaitingTraining;
  const bool awaiting_second_pass = stage_ == Stage::AwaitingSecondPass;
  const bool training_available = has_printer && !has_profile && !awaiting_second_pass;
  const bool can_restart =
      awaiting_training || awaiting_second_pass || workflow_.has_value() || calibration_workflow_.has_value() ||
      stage_ == Stage::Done;
  const bool can_show_first_pass = has_profile && stage_ == Stage::Setup;
  const bool ready_to_print = document_.has_value() && can_show_first_pass;
  const bool has_primary_action = training_available || awaiting_training || can_show_first_pass || awaiting_second_pass;
  const bool has_footer_content = can_restart || status_label_->isVisible();

  choose_pdf_button_->setEnabled(true);
  refresh_button_->setEnabled(true);
  train_button_->setEnabled(training_available && stage_ == Stage::Setup);
  train_button_->setVisible(training_available && stage_ == Stage::Setup);
  save_training_button_->setEnabled(awaiting_training);
  save_training_button_->setVisible(awaiting_training);
  first_pass_button_->setEnabled(ready_to_print);
  first_pass_button_->setVisible(can_show_first_pass);
  second_pass_button_->setEnabled(workflow_.has_value() && awaiting_second_pass);
  second_pass_button_->setVisible(awaiting_second_pass);
  action_card_->setVisible(has_primary_action);
  if (has_primary_action && action_card_->layout() != nullptr) {
    action_card_->setFixedHeight(action_card_->layout()->sizeHint().height());
  }
  reset_button_->setEnabled(can_restart);
  reset_button_->setVisible(can_restart);
  footer_container_->setVisible(has_footer_content);
  sync_window_size();
}

}  // namespace duplexprint::app
