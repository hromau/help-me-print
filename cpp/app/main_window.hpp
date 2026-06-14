#pragma once

#include <QMainWindow>

QT_BEGIN_NAMESPACE
class QLabel;
class QListWidget;
class QPushButton;
QT_END_NAMESPACE

namespace duplexprint::app {

class MainWindow : public QMainWindow {
 public:
  MainWindow();

 private:
  QLabel* status_label_ {nullptr};
  QListWidget* printers_list_ {nullptr};
  QPushButton* print_button_ {nullptr};
  QPushButton* retrain_button_ {nullptr};
};

}  // namespace duplexprint::app
