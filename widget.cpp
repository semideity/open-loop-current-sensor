#include "widget.h"
#include "ui_widget.h"
//#include <QDebug>

Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget)
{
    ui->setupUi(this);
    dbOK=true;
    ui->tab_tmr->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->tab_field->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->tab_output->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->tab_tmr, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(show_contextmenu_1(const QPoint&)));
    connect(ui->tab_field, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(show_contextmenu_2(const QPoint&)));
    connect(ui->tab_output, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(show_contextmenu_3(const QPoint&)));
    connect(ui->tab_tmr, SIGNAL(mouseMove(QMouseEvent*)), this, SLOT(my_mouseMove_1(QMouseEvent*)));
    connect(ui->tab_field, SIGNAL(mouseMove(QMouseEvent*)), this, SLOT(my_mouseMove_2(QMouseEvent*)));
    connect(ui->tab_output, SIGNAL(mouseMove(QMouseEvent*)), this, SLOT(my_mouseMove_3(QMouseEvent*)));

    // 检查数据库
    QSqlDatabase database;
    QString ErrMessage,sql_line;
    //database = QSqlDatabase::addDatabase("QSQLITE","SimuData.db");
    database = QSqlDatabase::addDatabase("QSQLITE", "sqlite1");
    database.setDatabaseName("SimuData.db");
    //QSqlQuery sql_query=QSqlQuery(database);
    int i;

    if (!database.open())
    {
        ErrMessage = "开环磁芯计算_v2.0（无法连接数据库）";
        this->setWindowTitle(ErrMessage);
        dbOK=false;
    } else {
        // 读取数据库版本
        QSqlQuery sql_query(database);
        sql_line = "SELECT * FROM data_version";
        sql_query.prepare(sql_line);
        if (!sql_query.exec())
        {
            ErrMessage = "开环磁芯计算_v2.0（查询数据库错误）";
            this->setWindowTitle(ErrMessage);
            dbOK=false;
        } else {
            sql_query.next();
            QString version="开环磁芯计算_v2.0（数据库版本：v"+sql_query.value(0).toString()+"）";
            this->setWindowTitle(version);
        }
        // 读取磁芯参数范围
        sql_line = "SELECT * FROM size_setting";
        sql_query.prepare(sql_line);
        if (!sql_query.exec())
        {
            ErrMessage = "开环磁芯计算_v2.0（查询数据库错误）";
            this->setWindowTitle(ErrMessage);
            // qDebug() << sql_query.lastError();
            dbOK=false;
        } else {
            QVector<double> line;
            size_num=0;
            while (sql_query.next())
            {
                size_num++;
                line.clear();
                for (i=1; i<=11; i++) {
                    line << sql_query.value(i).toDouble();
                }
                size_list << line;
            }
        }
        // 读取相对磁导率列表
        sql_line = "SELECT * FROM mur_setting";
        sql_query.prepare(sql_line);
        if (!sql_query.exec())
        {
            ErrMessage = "开环磁芯计算_v2.0（查询数据库错误）";
            this->setWindowTitle(ErrMessage);
            // qDebug() << sql_query.lastError();
            dbOK=false;
        } else {
            mur_num=0;
            while (sql_query.next())
            {
                mur_num++;
                mur_list << sql_query.value(1).toDouble();
            }
        }

        database.close();
    }
}

Widget::~Widget()
{
    delete ui;
}

bool Widget::isNum(QString str)
{
    int i=0;
    // 第一位为数字或负号
    if (!(str[i]=='0' || str[i]=='1' || str[i]=='2' || str[i]=='3' || str[i]=='4' || str[i]=='5' || str[i]=='6' || str[i]=='7' || str[i]=='8' || str[i]=='9' || str[i]=='-')) {
        return false;
    }
    // 最后一位为数字
    i=str.length()-1;
    if (!(str[i]=='0' || str[i]=='1' || str[i]=='2' || str[i]=='3' || str[i]=='4' || str[i]=='5' || str[i]=='6' || str[i]=='7' || str[i]=='8' || str[i]=='9')) {
        return false;
    }
    if (str.length() > 2) {
        // 其它位为数字、负号、小数点、e或E
        for (i = 1; i < str.length(); i++) {
            if (!(str[i]=='0' || str[i]=='1' || str[i]=='2' || str[i]=='3' || str[i]=='4' || str[i]=='5' || str[i]=='6' || str[i]=='7' || str[i]=='8' || str[i]=='9' || str[i]=='e' || str[i]=='E' || str[i]=='-' || str[i]=='.')) {
                return false;
            }
        }
        // 除第一位外负号必须出现在e或E后面
        for (i = 1; i < str.length(); i++) {
            if (str[i]=='-' && !(str[i-1]=='e' || str[i-1]=='E')) {
                return false;
            }
        }
        // e或E只能出现1次
        int e_num=0;
        for (i = 0; i < str.length(); i++) {
            if (str[i]=='e' || str[i]=='E') {
                e_num++;
            }
        }
        if (e_num>1) {
            return false;
        }
    }

    return true;
}

double Widget::B_interp(QVector<double> B_list, double mur)
{
    // 计算拟合曲线后的磁场值，拟合曲线 f(x)=a*x^b+c
    double N11,N12,N13,N21,N22,N23,N31,N32,N33,E1,E2,E3;
    double da,db,dc; // 拟合系数过程值
    double d_error; // 当前残差
    double tolerance_set=1E-4; // 目标残差
    double a,b,c; // 拟合系数
    int i,j,m,n;
    double out;

    // 开始拟合
    for (m=0; m<5; ++m) {
        // 设置初始值
        a = -1000-(double)m*1000;
        for (n=0; n<41; ++n) {
            b = -0.5-(double)n/100;
            c = 0;

            d_error=1; // 初始残差
            j=0; // 迭代步数
            while (d_error > tolerance_set) {
                if (j > 40) { // 达到最大迭代数，未收敛，返回
                    break;
                }
                // 初始化矩阵元
                N11=0;
                N12=0;
                N13=0;
                N21=0;
                N22=0;
                N23=0;
                N31=0;
                N32=0;
                N33=0;
                E1=0;
                E2=0;
                E3=0;
                // 计算矩阵元
                for (i = 0; i < mur_num; i++) {
                    // 系数矩阵
                    N11 += pow(mur_list[i],2*b);
                    N12 += a*pow(mur_list[i],2*b)*log(mur_list[i])+(a*pow(mur_list[i],b)+c-B_list[i])*pow(mur_list[i],b)*log(mur_list[i]);
                    N13 += pow(mur_list[i],b);
                    N21 += pow(mur_list[i],2*b)*log(mur_list[i]);
                    N22 += a*pow(mur_list[i],2*b)*pow(log(mur_list[i]),2)+(a*pow(mur_list[i],b)+c-B_list[i])*pow(mur_list[i],b)*pow(log(mur_list[i]),2);
                    N23 += pow(mur_list[i],b)*log(mur_list[i]);
                    N31 += pow(mur_list[i],b);
                    N32 += a*pow(mur_list[i],b)*log(mur_list[i]);
                    N33 += 1;
                    // 常数项
                    E1 += (a*pow(mur_list[i],b)+c-B_list[i])*pow(mur_list[i],b);
                    E2 += (a*pow(mur_list[i],b)+c-B_list[i])*pow(mur_list[i],b)*log(mur_list[i]);
                    E3 += a*pow(mur_list[i],b)+c-B_list[i];
                }

                if (N13*N22*N31-N12*N23*N31-N13*N21*N32+N11*N23*N32+N12*N21*N33-N11*N22*N33 == 0) {
                    break;
                } else {
                    da=(E3*N13*N22-E3*N12*N23-E2*N13*N32+E1*N23*N32+E2*N12*N33-E1*N22*N33)/(N13*N22*N31-N12*N23*N31-N13*N21*N32+N11*N23*N32+N12*N21*N33-N11*N22*N33);
                }
                if (-N13*N22*N31+N12*N23*N31+N13*N21*N32-N11*N23*N32-N12*N21*N33+N11*N22*N33 == 0) {
                    break;
                } else {
                    db=(E3*N13*N21-E3*N11*N23-E2*N13*N31+E1*N23*N31+E2*N11*N33-E1*N21*N33)/(-N13*N22*N31+N12*N23*N31+N13*N21*N32-N11*N23*N32-N12*N21*N33+N11*N22*N33);
                }
                if (N13*N22*N31-N12*N23*N31-N13*N21*N32+N11*N23*N32+N12*N21*N33-N11*N22*N33 == 0) {
                    break;
                } else {
                    dc=(E3*N12*N21-E3*N11*N22-E2*N12*N31+E1*N22*N31+E2*N11*N32-E1*N21*N32)/(N13*N22*N31-N12*N23*N31-N13*N21*N32+N11*N23*N32+N12*N21*N33-N11*N22*N33);
                }

                d_error=da*da+db*db+dc*dc;
                a=a-da;//<--------------------------------结果数据
                b=b-db;//<--------------------------------结果数据
                c=c-dc;//<--------------------------------结果数据
                j++;
            }
            // 输出结果
            if (d_error <= tolerance_set) {
                out=a*pow(mur,b)+c;
                if (out > 0 && out < 1000) {
                    return(out);
                }
            }
        }
    }
    return(-1);
}

double Widget::Bmax_interp(QVector<double> Bmax_list, double mur)
{
    // 计算拟合曲线后的最大磁场，拟合曲线 f(x)=a*x^b+c
    double N11,N12,N13,N21,N22,N23,N31,N32,N33,E1,E2,E3;
    double da,db,dc; // 拟合系数过程值
    double d_error; // 当前残差
    double tolerance_set=1E-4; // 目标残差
    double a,b,c; // 拟合系数
    int i,j,m,n;
    double out;

    //开始拟合
    for (m=0; m<5; ++m) {
        // 设置初始值
        a = -5000-(double)m*5000;
        for (n=0; n<41; ++n) {
            b = -0.5-(double)n/100;
            c = 0;

            d_error=1; // 初始残差
            j=0; // 迭代步数
            while (d_error > tolerance_set) {
                if (j > 40) { // 达到最大迭代数，未收敛，返回
                    break;
                }
                // 初始化矩阵元
                N11=0;
                N12=0;
                N13=0;
                N21=0;
                N22=0;
                N23=0;
                N31=0;
                N32=0;
                N33=0;
                E1=0;
                E2=0;
                E3=0;
                // 计算矩阵元
                for (i = 0; i < mur_num; i++) {
                    // 系数矩阵
                    N11 += pow(mur_list[i],2*b);
                    N12 += a*pow(mur_list[i],2*b)*log(mur_list[i])+(a*pow(mur_list[i],b)+c-Bmax_list[i])*pow(mur_list[i],b)*log(mur_list[i]);
                    N13 += pow(mur_list[i],b);
                    N21 += pow(mur_list[i],2*b)*log(mur_list[i]);
                    N22 += a*pow(mur_list[i],2*b)*pow(log(mur_list[i]),2)+(a*pow(mur_list[i],b)+c-Bmax_list[i])*pow(mur_list[i],b)*pow(log(mur_list[i]),2);
                    N23 += pow(mur_list[i],b)*log(mur_list[i]);
                    N31 += pow(mur_list[i],b);
                    N32 += a*pow(mur_list[i],b)*log(mur_list[i]);
                    N33 += 1;
                    // 常数项
                    E1 += (a*pow(mur_list[i],b)+c-Bmax_list[i])*pow(mur_list[i],b);
                    E2 += (a*pow(mur_list[i],b)+c-Bmax_list[i])*pow(mur_list[i],b)*log(mur_list[i]);
                    E3 += a*pow(mur_list[i],b)+c-Bmax_list[i];
                }

                if (N13*N22*N31-N12*N23*N31-N13*N21*N32+N11*N23*N32+N12*N21*N33-N11*N22*N33 == 0) {
                    break;
                } else {
                    da=(E3*N13*N22-E3*N12*N23-E2*N13*N32+E1*N23*N32+E2*N12*N33-E1*N22*N33)/(N13*N22*N31-N12*N23*N31-N13*N21*N32+N11*N23*N32+N12*N21*N33-N11*N22*N33);
                }
                if (-N13*N22*N31+N12*N23*N31+N13*N21*N32-N11*N23*N32-N12*N21*N33+N11*N22*N33 == 0) {
                    break;
                } else {
                    db=(E3*N13*N21-E3*N11*N23-E2*N13*N31+E1*N23*N31+E2*N11*N33-E1*N21*N33)/(-N13*N22*N31+N12*N23*N31+N13*N21*N32-N11*N23*N32-N12*N21*N33+N11*N22*N33);
                }
                if (N13*N22*N31-N12*N23*N31-N13*N21*N32+N11*N23*N32+N12*N21*N33-N11*N22*N33 == 0) {
                    break;
                } else {
                    dc=(E3*N12*N21-E3*N11*N22-E2*N12*N31+E1*N22*N31+E2*N11*N32-E1*N21*N32)/(N13*N22*N31-N12*N23*N31-N13*N21*N32+N11*N23*N32+N12*N21*N33-N11*N22*N33);
                }

                d_error=da*da+db*db+dc*dc;
                a=a-da;//<--------------------------------结果数据
                b=b-db;//<--------------------------------结果数据
                c=c-dc;//<--------------------------------结果数据
                j++;
            }
            // 输出结果
            if (d_error <= tolerance_set) {
                out=a*pow(mur,b)+c;
                if (out > 0 && out < 10000) {
                    return(out);
                }
            }
        }
    }
    return(-1);
}

int Widget::getMagField(double Rin, double Rout, double H, double Gap, double Mur, double &B, double &Bmax)
{
    // 查询获得磁场、最大磁场
    QSqlDatabase database;
    QString sql_line;
    database = QSqlDatabase::database("sqlite1");
    //QSqlQuery sql_query=QSqlQuery(database);

    QVector<double> B_list;
    QVector<double> Bmax_list;

    if (!database.open())
    {
        QMessageBox::critical(NULL,"错误","无法连接数据库");
    } else {
        QSqlQuery sql_query(database);
        for (int i=0; i<mur_num; ++i)
        {
            sql_line = "SELECT B,B_max FROM magnetic_field WHERE R_in=? AND R_out=? AND H=? AND Gap=? AND Mur=?";
            sql_query.prepare(sql_line);

            sql_query.bindValue(0, Rin);
            sql_query.bindValue(1, Rout);
            sql_query.bindValue(2, H);
            sql_query.bindValue(3, Gap);
            sql_query.bindValue(4, mur_list[i]);

            sql_query.exec();
            sql_query.next();
            B_list << sql_query.value(0).toDouble();
            Bmax_list << sql_query.value(1).toDouble();
        }
        database.close();
        // 计算得到插值结果
        B = B_interp(B_list, Mur);
        Bmax = Bmax_interp(Bmax_list, Mur);
        if (B == -1 || Bmax == -1) {
            QMessageBox::critical(NULL,"错误","插值拟合发生错误");
            return(-1);
        }
    }
    return(0);
}

void Widget::Interpolation(double left, double left_value, double right, double right_value, double pos, double &pos_value)
{
    if (right == left) {
        pos_value=left_value;
    } else {
        pos_value=left_value+(pos-left)*(right_value-left_value)/(right-left);
    }
}

int Widget::getInterMagField(double Rin, double Rout, double H, double Gap, double Mur, double &B, double &Bmax)
{
    // 获得插值磁场、最大磁场
    int range_index=0; // Rin所在范围的索引
    while (Rin > size_list[range_index][1]) {
        range_index++;
    }
    double Rin_min;
    if (range_index==0) {
        Rin_min=size_list[range_index][0];
    } else {
        Rin_min=size_list[range_index-1][1];
    }
    // Rin_x=Rin_min+floor((Rin-Rin_min)/Rin_step)*Rin_step;
    double Rin_1=Rin_min+floor((Rin-Rin_min)/size_list[range_index][2])*size_list[range_index][2]; //Rin左边界
    // Rin_x=Rin_min+ceil((Rin-Rin_min)/Rin_step)*Rin_step;
    double Rin_2=Rin_min+ceil((Rin-Rin_min)/size_list[range_index][2])*size_list[range_index][2]; //Rin右边界

    double width=Rout-Rin;
    // width_x=width_min+floor((width-width_min)/width_step)*width_step;
    double width_1=size_list[range_index][5]+floor((width-size_list[range_index][5])/size_list[range_index][7])*size_list[range_index][7]; //width左边界
    // width_x=width_min+ceil((width-width_min)/width_step)*width_step;
    double width_2=size_list[range_index][5]+ceil((width-size_list[range_index][5])/size_list[range_index][7])*size_list[range_index][7]; //width右边界

    // H_x=H_min+floor((H-H_min)/H_step)*H_step;
    double H_1=size_list[range_index][8]+floor((H-size_list[range_index][8])/size_list[range_index][10])*size_list[range_index][10]; //H左边界
    // H_x=H_min+ceil((H-H_min)/H_step)*H_step;
    double H_2=size_list[range_index][8]+ceil((H-size_list[range_index][8])/size_list[range_index][10])*size_list[range_index][10]; //H右边界

    // Gap_x=Gap_min+floor((Gap-Gap_min)/Gap_step)*Gap_step;
    double Gap_1=size_list[range_index][3]+floor((Gap-size_list[range_index][3])/size_list[range_index][4])*size_list[range_index][4]; //Gap左边界
    // Gap_x=Gap_min+ceil((Gap-Gap_min)/Gap_step)*Gap_step;
    double Gap_2=size_list[range_index][3]+ceil((Gap-size_list[range_index][3])/size_list[range_index][4])*size_list[range_index][4]; //Gap右边界

    double B_all[16][2];
    if (getMagField(Rin_1, Rin_1+width_1, H_1, Gap_1, Mur, B_all[0][0], B_all[0][1]) == -1) {
        return(-1);
    }
    if (getMagField(Rin_1, Rin_1+width_1, H_1, Gap_2, Mur, B_all[1][0], B_all[1][1]) == -1) {
        return(-1);
    }
    if (getMagField(Rin_1, Rin_1+width_1, H_2, Gap_1, Mur, B_all[2][0], B_all[2][1]) == -1) {
        return(-1);
    }
    if (getMagField(Rin_1, Rin_1+width_1, H_2, Gap_2, Mur, B_all[3][0], B_all[3][1]) == -1) {
        return(-1);
    }
    if (getMagField(Rin_1, Rin_1+width_2, H_1, Gap_1, Mur, B_all[4][0], B_all[4][1]) == -1) {
        return(-1);
    }
    if (getMagField(Rin_1, Rin_1+width_2, H_1, Gap_2, Mur, B_all[5][0], B_all[5][1]) == -1) {
        return(-1);
    }
    if (getMagField(Rin_1, Rin_1+width_2, H_2, Gap_1, Mur, B_all[6][0], B_all[6][1]) == -1) {
        return(-1);
    }
    if (getMagField(Rin_1, Rin_1+width_2, H_2, Gap_2, Mur, B_all[7][0], B_all[7][1]) == -1) {
        return(-1);
    }
    if (getMagField(Rin_2, Rin_2+width_1, H_1, Gap_1, Mur, B_all[8][0], B_all[8][1]) == -1) {
        return(-1);
    }
    if (getMagField(Rin_2, Rin_2+width_1, H_1, Gap_2, Mur, B_all[9][0], B_all[9][1]) == -1) {
        return(-1);
    }
    if (getMagField(Rin_2, Rin_2+width_1, H_2, Gap_1, Mur, B_all[10][0], B_all[10][1]) == -1) {
        return(-1);
    }
    if (getMagField(Rin_2, Rin_2+width_1, H_2, Gap_2, Mur, B_all[11][0], B_all[11][1]) == -1) {
        return(-1);
    }
    if (getMagField(Rin_2, Rin_2+width_2, H_1, Gap_1, Mur, B_all[12][0], B_all[12][1]) == -1) {
        return(-1);
    }
    if (getMagField(Rin_2, Rin_2+width_2, H_1, Gap_2, Mur, B_all[13][0], B_all[13][1]) == -1) {
        return(-1);
    }
    if (getMagField(Rin_2, Rin_2+width_2, H_2, Gap_1, Mur, B_all[14][0], B_all[14][1]) == -1) {
        return(-1);
    }
    if (getMagField(Rin_2, Rin_2+width_2, H_2, Gap_2, Mur, B_all[15][0], B_all[15][1]) == -1) {
        return(-1);
    }

    double B2_all[8][2];
    Interpolation(Gap_1, B_all[0][0], Gap_2, B_all[1][0], Gap, B2_all[0][0]);
    Interpolation(Gap_1, B_all[2][0], Gap_2, B_all[3][0], Gap, B2_all[1][0]);
    Interpolation(Gap_1, B_all[4][0], Gap_2, B_all[5][0], Gap, B2_all[2][0]);
    Interpolation(Gap_1, B_all[6][0], Gap_2, B_all[7][0], Gap, B2_all[3][0]);
    Interpolation(Gap_1, B_all[8][0], Gap_2, B_all[9][0], Gap, B2_all[4][0]);
    Interpolation(Gap_1, B_all[10][0], Gap_2, B_all[11][0], Gap, B2_all[5][0]);
    Interpolation(Gap_1, B_all[12][0], Gap_2, B_all[13][0], Gap, B2_all[6][0]);
    Interpolation(Gap_1, B_all[14][0], Gap_2, B_all[15][0], Gap, B2_all[7][0]);
    Interpolation(Gap_1, B_all[0][1], Gap_2, B_all[1][1], Gap, B2_all[0][1]);
    Interpolation(Gap_1, B_all[2][1], Gap_2, B_all[3][1], Gap, B2_all[1][1]);
    Interpolation(Gap_1, B_all[4][1], Gap_2, B_all[5][1], Gap, B2_all[2][1]);
    Interpolation(Gap_1, B_all[6][1], Gap_2, B_all[7][1], Gap, B2_all[3][1]);
    Interpolation(Gap_1, B_all[8][1], Gap_2, B_all[9][1], Gap, B2_all[4][1]);
    Interpolation(Gap_1, B_all[10][1], Gap_2, B_all[11][1], Gap, B2_all[5][1]);
    Interpolation(Gap_1, B_all[12][1], Gap_2, B_all[13][1], Gap, B2_all[6][1]);
    Interpolation(Gap_1, B_all[14][1], Gap_2, B_all[15][1], Gap, B2_all[7][1]);

    double B3_all[4][2];
    Interpolation(H_1, B2_all[0][0], H_2, B2_all[1][0], H, B3_all[0][0]);
    Interpolation(H_1, B2_all[2][0], H_2, B2_all[3][0], H, B3_all[1][0]);
    Interpolation(H_1, B2_all[4][0], H_2, B2_all[5][0], H, B3_all[2][0]);
    Interpolation(H_1, B2_all[6][0], H_2, B2_all[7][0], H, B3_all[3][0]);
    Interpolation(H_1, B2_all[0][1], H_2, B2_all[1][1], H, B3_all[0][1]);
    Interpolation(H_1, B2_all[2][1], H_2, B2_all[3][1], H, B3_all[1][1]);
    Interpolation(H_1, B2_all[4][1], H_2, B2_all[5][1], H, B3_all[2][1]);
    Interpolation(H_1, B2_all[6][1], H_2, B2_all[7][1], H, B3_all[3][1]);

    double B4_all[2][2];
    Interpolation(width_1, B3_all[0][0], width_2, B3_all[1][0], width, B4_all[0][0]);
    Interpolation(width_1, B3_all[2][0], width_2, B3_all[3][0], width, B4_all[1][0]);
    Interpolation(width_1, B3_all[0][1], width_2, B3_all[1][1], width, B4_all[0][1]);
    Interpolation(width_1, B3_all[2][1], width_2, B3_all[3][1], width, B4_all[1][1]);

    Interpolation(Rin_1, B4_all[0][0], Rin_2, B4_all[1][0], Rin, B);
    Interpolation(Rin_1, B4_all[0][1], Rin_2, B4_all[1][1], Rin, Bmax);

    return(0);
}

void Widget::on_calculate_clicked()
{
    // 计算
    if (dbOK==true) {
        switch (paraOK()) {
        case 1:
            QMessageBox::critical(NULL,"错误","磁芯内半径超出可计算范围：" + QString::number(size_list[0][0]) + "~" + QString::number(size_list[size_num-1][1]));
            return;
            break;
        case 2:
            QMessageBox::critical(NULL,"错误","磁芯宽度超出可计算范围：" + QString::number(size_list[0][5]) + "~" + QString::number(size_list[0][6]));
            return;
            break;
        case 3:
            QMessageBox::critical(NULL,"错误","气隙宽度超出可计算范围：" + QString::number(size_list[0][3]) + "~[内半径]");
            return;
            break;
        case 4:
            QMessageBox::critical(NULL,"错误","磁芯厚度超出可计算范围：" + QString::number(size_list[0][8]) + "~" + QString::number(size_list[0][9]));
            return;
            break;
        case 5:
            QMessageBox::critical(NULL,"错误","相对磁导率超出可计算范围：>=500");
            return;
            break;
        default:
            break;
        }
    } else {
        QMessageBox::critical(NULL,"错误","数据库错误");
        return;
    }

    // 清空结果
    ui->output_0->setText("");
    ui->output_1->setText("");
    ui->output_2->setText("");
    ui->output_3->setText("");
    QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

    // 画模型图
    // ================================
    ui->tab_model->plot=true;
    ui->tab_model->Rin=ui->s_Rin->text().toDouble();
    ui->tab_model->Rout=ui->s_Rout->text().toDouble();
    ui->tab_model->H=ui->s_H->text().toDouble();
    ui->tab_model->Gap=ui->s_Gap->text().toDouble();
    ui->tab_model->update();

    // 参数设置合法，开始计算
    // ================================
    int i;
    double vB,vBmax;
    double cal_Rin=ui->s_Rin->text().toDouble();
    double cal_Rout=ui->s_Rout->text().toDouble();
    double cal_Gap=ui->s_Gap->text().toDouble();
    double cal_Rmean=(sqrt(pow(cal_Rin,2)-pow(cal_Gap/2,2))+sqrt(pow(cal_Rout,2)-pow(cal_Gap/2,2)))/2;
    Bs=ui->s_Bs->text().toDouble()*1e4; // 饱和磁场
    Ct=ui->s_Ct->text().toDouble(); // 热膨胀系数

    // 插值计算结果
    if (getInterMagField(Rin, Rout, H, Gap, Mur, vB, vBmax) == -1) {
        ui->output_0->setText("--");
        ui->output_1->setText("--");
        ui->output_2->setText("--");
        ui->output_3->setText("--");
        return;
    }

    // 计算热膨胀误差
    int range_index=0; // Rin所在范围的索引
    while (Rin > size_list[range_index][1]) {
        range_index++;
    }
    double gap_step=size_list[range_index][4];
    double Rin_min;
    if (range_index==0) {
        Rin_min=size_list[range_index][0];
    } else {
        Rin_min=size_list[range_index-1][1];
    }
    double Rin_1=Rin_min+floor((Rin-Rin_min)/size_list[range_index][2])*size_list[range_index][2]; //Rin左边界
    double Gap_1=size_list[range_index][3]+floor((Gap-size_list[range_index][3])/size_list[range_index][4])*size_list[range_index][4]; //Gap左边界
    double Gap_2; //Rin右边界
    if (Gap_1 < Rin_1) {
        Gap_2=Gap_1+gap_step;
    } else {
        Gap_2=Gap_1;
        Gap_1=Gap_1-gap_step;
    }

    double B_left,B_right,tmp;
    if (getInterMagField(Rin, Rout, H, Gap_1, Mur, B_left, tmp) == -1) {
        ui->output_0->setText("--");
        ui->output_1->setText("--");
        ui->output_2->setText("--");
        ui->output_3->setText("--");
        return;
    }
    if (getInterMagField(Rin, Rout, H, Gap_2, Mur, B_right, tmp) == -1) {
        ui->output_0->setText("--");
        ui->output_1->setText("--");
        ui->output_2->setText("--");
        ui->output_3->setText("--");
        return;
    }
    double delta_vB_gap=(B_left-B_right)/gap_step;
    double delta_gap=Ct*Gap*100; // 变化100摄氏度，气隙宽度变化(mm)
    double B_percent=delta_vB_gap*delta_gap/vB*100;

    // ========== current-field curve ======================================
    double BpI=vB/10; // 单位电流磁场[Gs/A]
    double Is=1.1*Bs/(vBmax/10); // 饱和电流[A]
    double current_max=floor(1.5*Is/10)*10; // 作图电流范围[A]
    double delta_I=current_max/50.0; // 电流步进[A]

    Current.clear();
    Current.resize(101);
    Mag.clear();
    Mag.resize(101);
    // 正半轴
    int step=1;
    for (i = 50; i < 101; i++) {
        Current[i]=delta_I*((double)i-50);
        if (Current[i] <= Is) { // 不饱和
            Mag[i]=BpI*Current[i];
        } else { // 饱和
            Mag[i]=BpI*Is+delta_I*(double)step*(2/cal_Rmean);
            step++;
        }
    }
    // 负半轴
    for (i = 0; i < 50; i++) {
        Current[i]=-Current[100-i];
        Mag[i]=-Mag[100-i];
    }
    // 画图
    plot_field_curve();

    // ========== current-output curve ======================================
    double Hs=ui->s_Hs->text().toDouble();
    current_max=floor(3*Hs/BpI); // 作图电流范围[A]
    delta_I=current_max/50.0; // 电流步进[A]

    Vout.clear();
    Vout.resize(101);

    step=1;
    for (i = 50; i < 101; i++) {
        Current[i]=delta_I*((double)i-50);
        if (Current[i] <= Is) {
            Mag[i]=vB/10.0*Current[i];
        } else {
            Mag[i]=vB/10.0*Is+delta_I*(double)step*(2/cal_Rmean);
            step++;
        }
    }
    for (i = 0; i < 50; i++) {
        Current[i]=-Current[100-i];
        Mag[i]=-Mag[100-i];
    }

    double R1,R2,Vp,Vn;
    double Rmax=ui->s_Rmax->text().toDouble();
    double Rmin=ui->s_Rmin->text().toDouble();
    double Hoff=ui->s_Hoff->text().toDouble();
    Vcc=ui->s_Vcc->text().toDouble();
    if (ui->comb_type->currentIndex() == 0) { // 半桥
        for (i = 0; i < 101; i++) {
            R1=Rmax+(Rmin-Rmax)/(1+exp(2*(Mag[i]-Hoff)/Hs));
            R2=Rmax+(Rmin-Rmax)/(1+exp(2*(-Mag[i]-Hoff)/Hs));
            Vout[i]=Vcc*R1/(R1+R2);
        }
    } else { // 全桥
        for (i = 0; i < 101; i++) {
            R1=Rmax+(Rmin-Rmax)/(1+exp(2*(Mag[i]-Hoff)/Hs));
            R2=Rmax+(Rmin-Rmax)/(1+exp(2*(-Mag[i]-Hoff)/Hs));
            Vp=Vcc*R1/(R1+R2);
            Vn=Vcc*R2/(R1+R2);
            Vout[i]=Vp-Vn;
        }
    }

    // 画图
    plot_output_curve();

    // ========== TMR curve ======================================
    double TMR_R1,TMR_R2,TMR_Vp,TMR_Vn;
    B_range=ceil((Hs+fabs(Hoff))*3/100)*100;
    TMR_B.clear();
    TMR_Vout.clear();
    for (i=0; i<=400; i++) {
        TMR_B << -B_range+(double)i*2*B_range/400;
    }
    if (ui->comb_type->currentIndex() == 0) {
        // 半桥连接
        for (i=0; i<=400; i++) {
            TMR_R1=Rmax+(Rmin-Rmax)/(1+exp(2*( TMR_B[i]-Hoff)/Hs));
            TMR_R2=Rmax+(Rmin-Rmax)/(1+exp(2*(-TMR_B[i]-Hoff)/Hs));
            TMR_Vout << Vcc*TMR_R1/(TMR_R1+TMR_R2);
        }
    } else {
        // 全桥连接
        for (i=0; i<=400; i++) {
            TMR_R1=Rmax+(Rmin-Rmax)/(1+exp(2*( TMR_B[i]-Hoff)/Hs));
            TMR_R2=Rmax+(Rmin-Rmax)/(1+exp(2*(-TMR_B[i]-Hoff)/Hs));
            TMR_Vp=Vcc*TMR_R1/(TMR_R1+TMR_R2);
            TMR_Vn=Vcc*TMR_R2/(TMR_R1+TMR_R2);
            TMR_Vout << TMR_Vp-TMR_Vn;
        }
    }
    // 画图
    plot_tmr_curve();

    // ========== output results ======================================
    double TMR_sen;
    if (ui->comb_type->currentIndex() == 0) {
        // 半桥连接
        TMR_sen = 1e3*exp(2*Hoff/Hs)*(Rmax-Rmin)/(1+exp(2*Hoff/Hs))/Hs/(Rmax+exp(2*Hoff/Hs)*Rmin);
    } else {
        // 全桥连接
        TMR_sen = 2e3*exp(2*Hoff/Hs)*(Rmax-Rmin)/(1+exp(2*Hoff/Hs))/Hs/(Rmax+exp(2*Hoff/Hs)*Rmin);
    }
    ui->output_0->setText(QString::number(TMR_sen,'f',2));
    ui->output_1->setText(QString::number(BpI,'f',2));
    ui->output_2->setText(QString::number(Is,'f',0));
    ui->output_3->setText(QString::number(B_percent,'f',2)+" %");
}

void Widget::on_view_model_clicked()
{
    // 查看模型
    if (dbOK==false) {
        QMessageBox::critical(NULL,"错误","数据库错误");
        return;
    }
    switch (paraOK()) {
    case 1:
        QMessageBox::critical(NULL,"错误","磁芯内半径超出可计算范围：" + QString::number(size_list[0][0]) + "~" + QString::number(size_list[size_num-1][1]));
        return;
        break;
    case 2:
        QMessageBox::critical(NULL,"错误","磁芯宽度超出可计算范围：" + QString::number(size_list[0][5]) + "~" + QString::number(size_list[0][6]));
        return;
        break;
    case 3:
        QMessageBox::critical(NULL,"错误","气隙宽度超出可计算范围：" + QString::number(size_list[0][3]) + "~[内半径]");
        return;
        break;
    case 4:
        QMessageBox::critical(NULL,"错误","磁芯厚度超出可计算范围：" + QString::number(size_list[0][8]) + "~" + QString::number(size_list[0][9]));
        return;
        break;
    case 5:
        QMessageBox::critical(NULL,"错误","相对磁导率超出可计算范围：>=500");
        return;
        break;
    default:
        ui->tabWidget->setCurrentIndex(0);
        ui->tab_model->plot=true;
        ui->tab_model->Rin=ui->s_Rin->text().toDouble();
        ui->tab_model->Rout=ui->s_Rout->text().toDouble();
        ui->tab_model->H=ui->s_H->text().toDouble();
        ui->tab_model->Gap=ui->s_Gap->text().toDouble();
        ui->tab_model->update();
        break;
    }
}

void Widget::on_show_curve_clicked()
{
    // 查看TMR特性曲线
    Vcc=ui->s_Vcc->text().toDouble();
    double Hoff=ui->s_Hoff->text().toDouble();
    double Hs=ui->s_Hs->text().toDouble();
    double Rmin=ui->s_Rmin->text().toDouble();
    double Rmax=ui->s_Rmax->text().toDouble();
    int i;
    TMR_B.clear();
    TMR_Vout.clear();
    double TMR_R1,TMR_R2,TMR_Vp,TMR_Vn;
    B_range=ceil((Hs+fabs(Hoff))*3/100)*100;
    for (i=0; i<=400; i++) {
        TMR_B << -B_range+(double)i*2*B_range/400;
    }
    if (ui->comb_type->currentIndex() == 0) {
        // 半桥连接
        for (i=0; i<=400; i++) {
            TMR_R1=Rmax+(Rmin-Rmax)/(1+exp(2*( TMR_B[i]-Hoff)/Hs));
            TMR_R2=Rmax+(Rmin-Rmax)/(1+exp(2*(-TMR_B[i]-Hoff)/Hs));
            TMR_Vout << Vcc*TMR_R1/(TMR_R1+TMR_R2);
        }
    } else {
        // 全桥连接
        for (i=0; i<=400; i++) {
            TMR_R1=Rmax+(Rmin-Rmax)/(1+exp(2*( TMR_B[i]-Hoff)/Hs));
            TMR_R2=Rmax+(Rmin-Rmax)/(1+exp(2*(-TMR_B[i]-Hoff)/Hs));
            TMR_Vp=Vcc*TMR_R1/(TMR_R1+TMR_R2);
            TMR_Vn=Vcc*TMR_R2/(TMR_R1+TMR_R2);
            TMR_Vout << TMR_Vp-TMR_Vn;
        }
    }
    // 画图
    plot_tmr_curve();
    ui->tabWidget->setCurrentIndex(1);
}

void Widget::on_rb_Rmax_clicked()
{
    // 设置Rmax
    ui->s_Rmax->setEnabled(true);
    ui->s_MRr->setEnabled(false);
}

void Widget::on_rb_MRr_clicked()
{
    // 设置MRr
    ui->s_Rmax->setEnabled(false);
    ui->s_MRr->setEnabled(true);
}

int Widget::paraOK()
{
    // 检查参数合法性
    /*
    错误代码：
    1) Rin超出范围
    2) 宽度超出范围
    3) Gap超出范围
    4) H超出范围
    5) Mur小于500
    */
    Rin=ui->s_Rin->text().toDouble();
    Rout=ui->s_Rout->text().toDouble();
    H=ui->s_H->text().toDouble();
    Gap=ui->s_Gap->text().toDouble();
    Mur=ui->s_Mur->text().toDouble();
    int i;
    for (i=0; i<size_num; i++) {
        if (Rin < size_list[0][0] || Rin > size_list[size_num-1][1]) {
            return(1);
        }
        if (Rout-Rin < size_list[i][5] || Rout-Rin > size_list[i][6]) {
            return(2);
        }
        if (Gap < size_list[i][3] || Gap > Rin) {
            return(3);
        }
        if (H < size_list[i][8] || H > size_list[i][9]) {
            return(4);
        }
        if (Mur < mur_list[0]) {
            return(5);
        }
    }
    return(0);
}

void Widget::plot_tmr_curve()
{
    // 画TMR特性曲线
    ui->tab_tmr->clearGraphs();
    ui->tab_tmr->addGraph(); // 添加一条曲线
    ui->tab_tmr->graph(0)->setData(TMR_B,TMR_Vout); // 设置曲线数据
    QPen pen; // 画笔
    pen.setWidth(2); // 线宽
    pen.setColor(Qt::red); // 颜色
    ui->tab_tmr->graph(0)->setPen(pen);

    // 画图设置
    ui->tab_tmr->xAxis->setRange(-B_range, B_range);
    if (ui->comb_type->currentIndex() == 0) {
        ui->tab_tmr->yAxis->setRange(0.0,Vcc);
    } else {
        ui->tab_tmr->yAxis->setRange(-Vcc,Vcc);
    }
    ui->tab_tmr->xAxis->setLabel("磁场 (Oe)"); // x轴标题
    ui->tab_tmr->yAxis->setLabel("输出 (V)"); // y轴标题
    // 标题
    if (ui->tab_tmr->plotLayout()->rowCount() == 1) {
        ui->tab_tmr->plotLayout()->insertRow(0);
    } else {
        ui->tab_tmr->plotLayout()->removeAt(0);
    }
    if (ui->comb_type->currentIndex() == 0) {
        ui->tab_tmr->plotLayout()->addElement(0, 0, new QCPTextElement(ui->tab_tmr, "TMR半桥输出"));
    } else {
        ui->tab_tmr->plotLayout()->addElement(0, 0, new QCPTextElement(ui->tab_tmr, "TMR全桥输出"));
    }

    // 重新绘图
    ui->tab_tmr->replot();
}

void Widget::plot_field_curve()
{
    // 画磁场曲线图
    ui->tab_field->clearGraphs();
    ui->tab_field->addGraph(); // 添加一条曲线
    ui->tab_field->graph(0)->setData(Current,Mag); // 设置曲线数据
    QPen pen; // 画笔
    pen.setWidth(2); // 线宽
    pen.setColor(Qt::red); // 颜色
    ui->tab_field->graph(0)->setPen(pen);

    // 画图设置
    ui->tab_field->xAxis->setRange(Current.first(), Current.last());
    ui->tab_field->yAxis->setRange(-ceil(Mag.last()/100)*120.0,ceil(Mag.last()/100)*120.0);
    ui->tab_field->xAxis->setLabel("电流 (A)"); // x轴标题
    ui->tab_field->yAxis->setLabel("磁场 (Gs)"); // y轴标题
    // 标题
    if (ui->tab_field->plotLayout()->rowCount() == 1) {
        ui->tab_field->plotLayout()->insertRow(0);
    } else {
        ui->tab_field->plotLayout()->removeAt(0);
    }
    ui->tab_field->plotLayout()->addElement(0, 0, new QCPTextElement(ui->tab_field, "磁场-电流 曲线"));

    // 重新绘图
    ui->tab_field->replot();
    ui->tabWidget->setCurrentIndex(2);
}

void Widget::plot_output_curve()
{
    // 画磁场曲线图
    ui->tab_output->clearGraphs();
    ui->tab_output->addGraph(); // 添加一条曲线
    ui->tab_output->graph(0)->setData(Current,Vout); // 设置曲线数据
    QPen pen; // 画笔
    pen.setWidth(2); // 线宽
    pen.setColor(Qt::red); // 颜色
    ui->tab_output->graph(0)->setPen(pen);

    // 画图设置
    ui->tab_output->xAxis->setRange(Current.first(), Current.last());
    if (ui->comb_type->currentIndex() == 0) { // 半桥
        ui->tab_output->yAxis->setRange(0,ceil(Vout.last())*1.2);
    } else { // 全桥
        ui->tab_output->yAxis->setRange(-ceil(Vout.last())*1.2,ceil(Vout.last())*1.2);
    }
    ui->tab_output->xAxis->setLabel("电流 (A)"); // x轴标题
    ui->tab_output->yAxis->setLabel("输出 (V)"); // y轴标题
    // 标题
    if (ui->tab_output->plotLayout()->rowCount() == 1) {
        ui->tab_output->plotLayout()->insertRow(0);
    } else {
        ui->tab_output->plotLayout()->removeAt(0);
    }
    if (ui->comb_type->currentIndex() == 0) {
        ui->tab_output->plotLayout()->addElement(0, 0, new QCPTextElement(ui->tab_output, "半桥输出-电流 曲线"));
    } else {
        ui->tab_output->plotLayout()->addElement(0, 0, new QCPTextElement(ui->tab_output, "全桥输出-电流 曲线"));
    }

    // 重新绘图
    ui->tab_output->replot();
}

void Widget::show_contextmenu_1(const QPoint &pos)
{
    // TMR特性图右键菜单
    QMenu *cmenu = new QMenu(this);
    QAction *act1 = cmenu->addAction("保存图片");
    connect(act1, SIGNAL(triggered(bool)), this, SLOT(save_pic_1()));
    QPoint globalPos = ui->tab_tmr->mapToGlobal(pos);
    cmenu->exec(globalPos);

    delete cmenu;
}

void Widget::show_contextmenu_2(const QPoint &pos)
{
    // 电流-磁场图右键菜单
    QMenu *cmenu = new QMenu(this);
    QAction *act1 = cmenu->addAction("保存图片");
    connect(act1, SIGNAL(triggered(bool)), this, SLOT(save_pic_2()));
    QPoint globalPos = ui->tab_field->mapToGlobal(pos);
    cmenu->exec(globalPos);

    delete cmenu;
}

void Widget::show_contextmenu_3(const QPoint &pos)
{
    // 电流-输出图右键菜单
    QMenu *cmenu = new QMenu(this);
    QAction *act1 = cmenu->addAction("保存图片");
    connect(act1, SIGNAL(triggered(bool)), this, SLOT(save_pic_3()));
    QPoint globalPos = ui->tab_output->mapToGlobal(pos);
    cmenu->exec(globalPos);

    delete cmenu;
}

void Widget::save_pic_1()
{
    // 保存TMR特性图
    QString filename;
    filename=QFileDialog::getSaveFileName(this,"Save To BMP","/","Bit Map File(*.bmp)") + ".bmp";
    ui->tab_tmr->saveBmp(filename,ui->tab_tmr->width(),ui->tab_tmr->height());
}

void Widget::save_pic_2()
{
    // 保存电流-磁场图
    QString filename;
    filename=QFileDialog::getSaveFileName(this,"Save To BMP","/","Bit Map File(*.bmp)") + ".bmp";
    ui->tab_field->saveBmp(filename,ui->tab_field->width(),ui->tab_field->height());
}

void Widget::save_pic_3()
{
    // 保存电流-输出图
    QString filename;
    filename=QFileDialog::getSaveFileName(this,"Save To BMP","/","Bit Map File(*.bmp)") + ".bmp";
    ui->tab_output->saveBmp(filename,ui->tab_output->width(),ui->tab_output->height());
}


void Widget::on_s_Rmax_editingFinished()
{
    // 修改Rmax
    double v_MRr;
    QString tmp=ui->s_Rmax->text();
    if (tmp.length() > 0 && isNum(tmp)) {
        v_MRr=(tmp.toDouble()/ui->s_Rmin->text().toDouble()-1)*100.0;
        ui->s_MRr->setText(QString::number(v_MRr,'f',0));
    }
}

void Widget::on_s_MRr_editingFinished()
{
    // 修改MRr
    double v_Rmax;
    QString tmp=ui->s_MRr->text();
    if (tmp.length() > 0 && isNum(tmp)) {
        v_Rmax=(tmp.toDouble()/100.0+1)*ui->s_Rmin->text().toDouble();
        ui->s_Rmax->setText(QString::number(v_Rmax,'f',2));
    }
}

void Widget::my_mouseMove_1(QMouseEvent* event)
{
    // TMR特性曲线鼠标捕获数据
    if ( event->pos().rx() > 0 && event->pos().rx() < ui->tab_tmr->width()  && event->pos().ry() > 0 && event->pos().ry() < ui->tab_tmr->height() ) {
        ui->tab_tmr->setCursor(Qt::CrossCursor);
    } else {
        ui->tab_tmr->setCursor(Qt::ArrowCursor);      //范围之外变回原来形状
    }

    QPoint mousepos = event->globalPos();
    mousepos += QPoint(10,-80);

    int x_pos = event->pos().x();
    int y_pos = event->pos().y();

    double x_val = ui->tab_tmr->xAxis->pixelToCoord(x_pos);
    double y_val = ui->tab_tmr->yAxis->pixelToCoord(y_pos);

    QToolTip::showText(mousepos,QString("输出： %1\n磁场： %2").arg(y_val).arg(x_val));
}

void Widget::my_mouseMove_2(QMouseEvent* event)
{
    // ”磁场-电流“曲线鼠标捕获数据
    if ( event->pos().rx() > 0 && event->pos().rx() < ui->tab_field->width()  && event->pos().ry() > 0 && event->pos().ry() < ui->tab_field->height() ) {
        ui->tab_field->setCursor(Qt::CrossCursor);
    } else {
        ui->tab_field->setCursor(Qt::ArrowCursor);      //范围之外变回原来形状
    }

    QPoint mousepos = event->globalPos();
    mousepos += QPoint(10,-80);

    int x_pos = event->pos().x();
    int y_pos = event->pos().y();

    double x_val = ui->tab_field->xAxis->pixelToCoord(x_pos);
    double y_val = ui->tab_field->yAxis->pixelToCoord(y_pos);

    QToolTip::showText(mousepos,QString("磁场： %1\n电流： %2").arg(y_val).arg(x_val));
}

void Widget::my_mouseMove_3(QMouseEvent* event)
{
    // “输出-电流”曲线鼠标捕获数据
    if ( event->pos().rx() > 0 && event->pos().rx() < ui->tab_output->width()  && event->pos().ry() > 0 && event->pos().ry() < ui->tab_output->height() ) {
        ui->tab_output->setCursor(Qt::CrossCursor);
    } else {
        ui->tab_output->setCursor(Qt::ArrowCursor);      //范围之外变回原来形状
    }

    QPoint mousepos = event->globalPos();
    mousepos += QPoint(10,-80);

    int x_pos = event->pos().x();
    int y_pos = event->pos().y();

    double x_val = ui->tab_output->xAxis->pixelToCoord(x_pos);
    double y_val = ui->tab_output->yAxis->pixelToCoord(y_pos);

    QToolTip::showText(mousepos,QString("输出： %1\n电流： %2").arg(y_val).arg(x_val));
}
