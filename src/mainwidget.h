#ifndef MAINWIDGET_H
#define MAINWIDGET_H

#include <QDragEnterEvent>
#include <QMimeData>
#include <QTcpSocket>
#include <QWidget>

namespace Ui {
    class MainWidget;
}

class MainWidget : public QWidget {
    Q_OBJECT
public:
    explicit MainWidget(const QString &savePath, QTcpSocket *socket, QWidget *parent = nullptr);
    ~MainWidget();
private:
    Ui::MainWidget *ui;
protected:
    void dragEnterEvent(QDragEnterEvent *e);
    void dropEvent(QDropEvent *e);
};

#endif // MAINWIDGET_H
