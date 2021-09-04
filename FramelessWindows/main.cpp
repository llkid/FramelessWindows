#include <shellscalingapi.h>
#include <qdebug.h>
#include <qscreen.h>
#include <qtimer.h>

#include <QSplashScreen>
#include <QtWidgets/QApplication>

#include "FramelessWindows.h"

#pragma execution_character_set("utf-8")
#pragma comment(lib, "Shcore.lib")

// class Application : public QApplication
//{
// public:
//    QWidget* widget = nullptr;
//
//    Application(int& argc, char** argv)
//        : QApplication(argc, argv)
//    {
//
//    }
//};

int main(int argc, char *argv[]) {
  SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
  // 自定义dpi缩放不能设置AA_EnableHighDpiScaling属性
  //QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
  QApplication a(argc, argv);

  /*QPixmap pixmap("D:/表情包/v2-78c30881ee13bc6389d56575f016574d_1440w.jpg");
  QSplashScreen splash(pixmap);
  splash.show();
  a.processEvents();

  QScopedPointer<QWidget> widget(new FramelessWindows);

      QTimer::singleShot(3000, [&] {
              splash.close();
              widget->show();
              a.beep();
              });*/

  FramelessWindows w;
  w.show();

  return a.exec();
}
