#include "FramelessWindows.h"
#include <QtWidgets/QApplication>

#include <qdebug.h>
#include <qscreen.h>
#include <QSplashScreen>
#include <qtimer.h>

#pragma execution_character_set("utf-8")

//class Application : public QApplication
//{
//public:
//    QWidget* widget = nullptr;
//
//    Application(int& argc, char** argv)
//        : QApplication(argc, argv)
//    {
//
//    }
//};

int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication a(argc, argv);

    /*QPixmap pixmap("D:/±íÇé°ü/v2-78c30881ee13bc6389d56575f016574d_1440w.jpg");
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
