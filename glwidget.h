#ifndef GLWIDGET_H
#define GLWIDGET_H

#include <QWidget>
#include <QOpenGLWidget>
#include <QMouseEvent>
#include <QMatrix4x4>
#include "draw.h"
#include "ArcBall.h"
#include "widget.h"

class GLWidget : public QOpenGLWidget
{
    Q_OBJECT
public:
    explicit GLWidget(QWidget *parent = nullptr);

    // 绘图开关
    bool plot;
    double Rin,Rout,H,Gap;
    QPoint m_lastPos;
    QPoint m_pressPos;

private:
    // 平移量
    float m_xMov;
    float m_yMov;
    float m_zMov;
    // 缩放比例
    float m_scale;

protected:
    void initializeGL();
    void resizeGL(int w, int h);
    void paintGL();
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event);

signals:

public slots:
};

#endif // GLWIDGET_H
