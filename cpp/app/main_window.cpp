#include "main_window.hpp"

#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

namespace duplexprint::app {

MainWindow::MainWindow() {
  setWindowTitle("DuplexPrint");
  resize(440, 520);

  auto* central = new QWidget(this);
  auto* layout = new QVBoxLayout(central);

  auto* title = new QLabel("Manual duplex printing without Electron", central);
  status_label_ = new QLabel("C++ core is active. Connect printer discovery and cloud profiles next.", central);
  printers_list_ = new QListWidget(central);
  print_button_ = new QPushButton("Print", central);
  retrain_button_ = new QPushButton("Retrain printer", central);

  printers_list_->addItem("HP Smart Tank 580  |  Cloud profile available");
  printers_list_->addItem("Brother HL-L2350DW  |  Already trained");
  printers_list_->addItem("Generic Office Printer  |  Needs training");

  print_button_->setEnabled(true);
  retrain_button_->setEnabled(true);

  layout->addWidget(title);
  layout->addWidget(status_label_);
  layout->addWidget(printers_list_);
  layout->addWidget(print_button_);
  layout->addWidget(retrain_button_);

  setCentralWidget(central);
}

}  // namespace duplexprint::app
