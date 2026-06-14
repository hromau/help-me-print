#include <QApplication>

#include "main_window.hpp"

int main(int argc, char* argv[]) {
  QApplication app(argc, argv);

  duplexprint::app::MainWindow window;
  window.show();

  return app.exec();
}
