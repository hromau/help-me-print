#include <QApplication>
#include <QIcon>

#include "main_window.hpp"

int main(int argc, char* argv[]) {
  QApplication app(argc, argv);
  app.setWindowIcon(QIcon(":/icons/app-icon-256.png"));

  duplexprint::app::MainWindow window;
  window.show();

  return app.exec();
}
