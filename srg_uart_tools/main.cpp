#include <QtGui/QApplication>
#include <QDesktopWidget>
#include "mainwindow.h"

void center(QWidget &widget)
{
  int scrn = 0;
  QWidget *w = widget.topLevelWidget();

  if(w)
       scrn = QApplication::desktop()->screenNumber(w);
  else if(QApplication::desktop()->isVirtualDesktop())
       scrn = QApplication::desktop()->screenNumber(QCursor::pos());
  else
      scrn = QApplication::desktop()->screenNumber(&widget);

  QRect desk(QApplication::desktop()->availableGeometry(scrn));

  widget.move((desk.width() - widget.width()) / 2,
        (desk.height() - widget.height()) / 2);

}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    center(w);

    return a.exec();
}
