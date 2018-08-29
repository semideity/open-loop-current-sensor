#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QMessageBox>
#include <QResizeEvent>
#include "qcustomplot.h"

namespace Ui {
class Widget;
}

class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(QWidget *parent = 0);
    ~Widget();

private slots:
    void on_calculate_clicked();
    void on_view_model_clicked();
    void on_show_curve_clicked();
    void on_rb_Rmax_clicked();
    void on_rb_MRr_clicked();
    void show_contextmenu_1(const QPoint& pos);
    void show_contextmenu_2(const QPoint& pos);
    void show_contextmenu_3(const QPoint& pos);
    void save_pic_1();
    void save_pic_2();
    void save_pic_3();
    void on_s_Rmax_editingFinished();
    void on_s_MRr_editingFinished();
    void my_mouseMove_1(QMouseEvent* event);
    void my_mouseMove_2(QMouseEvent* event);
    void my_mouseMove_3(QMouseEvent* event);

private:
    Ui::Widget *ui;
    void plot_tmr_curve();
    void plot_field_curve();
    void plot_output_curve();
    bool isNum(QString str);
    int paraOK();
    double B_interp(QVector<double> B_list, double mur);
    double Bmax_interp(QVector<double> Bmax_list, double mur);
    int getMagField(double Rin, double Rout, double H, double Gap, double Mur, double &B, double &Bmax);
    int getInterMagField(double Rin, double Rout, double H, double Gap, double Mur, double &B, double &Bmax);
    void Interpolation(double left, double left_value, double right, double right_value, double pos, double &pos_value);
    QVector<double> TMR_B;
    QVector<double> TMR_Vout;
    QVector<QVector<double>> size_list; // 磁芯尺寸参数范围
    QVector<double> mur_list; // 相对磁导率列表
    int size_num; // 尺寸设置条件数
    int mur_num; // 相对磁导率列表数
    bool dbOK; // 数据库连接正常
    QVector<double> Current; // 电流
    QVector<double> Mag; // 磁场
    QVector<double> Vout; // 输出

    double Rin; // 磁芯内半径
    double Rout; // 磁芯外半径
    double H; // 磁芯厚度
    double Gap; // 磁芯气隙宽度
    double Mur; // 相对磁导率
    double Bs; // 饱和磁场
    double Ct; // 热膨胀系数
    double Pr; // 泊松比

    double Vcc,B_range;
};

#endif // WIDGET_H
