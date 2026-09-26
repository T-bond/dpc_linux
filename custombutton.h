#ifndef CUSTOMBUTTON_H
#define CUSTOMBUTTON_H

#include <QObject>
#include <QWidget>
#include <QPushButton>
#include <QPaintEvent>
#include <QEvent>

// tab button
class tabButton : public QPushButton
{
    Q_OBJECT
public:
    tabButton(QString image_path, QWidget *parent, QString text = "");
    ~tabButton();

    // set tab check
    void setTabCheck(bool check);

protected:
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    void enterEvent(QEnterEvent*);
#else
    void enterEvent(QEvent*);
#endif
    void leaveEvent(QEvent*);
    void paintEvent(QPaintEvent* e);
    void mousePressEvent(QMouseEvent *e);
    void mouseReleaseEvent(QMouseEvent *e);

private:
    QList<QPixmap>         m_list_pixmap;
    int                                   m_index;
    bool                                m_enter;
    bool                                m_check;
    QString                          m_text;
};

#endif // CUSTOMBUTTON_H
