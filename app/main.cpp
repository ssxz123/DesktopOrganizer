#include "main_window.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // 配置目录：%APPDATA%/DesktopOrganizer
    QCoreApplication::setOrganizationName(QStringLiteral("YMH"));
    QCoreApplication::setApplicationName(QStringLiteral("DesktopOrganizer"));
    QCoreApplication::setApplicationVersion(QStringLiteral("0.1"));

    QApplication::setWindowIcon(QIcon(QStringLiteral(":/icons/app.svg")));

    MainWindow window;
    window.show();
    return QApplication::exec();
}
