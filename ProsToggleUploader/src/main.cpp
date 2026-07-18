#include "MainWindow.hpp"

#include <QApplication>
#include <QCoreApplication>
#include <QIcon>

int main(int argc, char *argv[]) {
    QApplication application(argc, argv);

    QCoreApplication::setOrganizationName(QStringLiteral("LocalTools"));
    QCoreApplication::setApplicationName(QStringLiteral("ProsToggleUploader"));
    QCoreApplication::setApplicationVersion(QStringLiteral("1.0.0"));

    MainWindow window;
    window.show();

    return application.exec();
}
