#include "draw.h"
#include <math.h>

// 绘制坐标系
void DrawAxis(GLfloat axis_length)
{
	//绘制x、y、z坐标轴

    glColor3f(1.0f, 0.0f, 0.0f);//指定线的颜色,红色
	glBegin(GL_LINES);
	{
        //axis_length=-axis_length;// 解决x坐标反向问题
		// x-axis
		glVertex3f(0.0f, 0.0f, 0.0f);
		glVertex3f(axis_length, 0.0f, 0.0f);

		// x-axis arrow
		glVertex3f(axis_length, 0.0f, 0.0f);
		glVertex3f(0.95*axis_length, 0.03*axis_length, 0.0f);
		glVertex3f(axis_length, 0.0f, 0.0f);
		glVertex3f(0.95*axis_length, -0.03*axis_length, 0.0f);
		glVertex3f(axis_length, 0.0f, 0.0f);
		glVertex3f(0.95*axis_length, 0.0f, 0.03*axis_length);
		glVertex3f(axis_length, 0.0f, 0.0f);
		glVertex3f(0.95*axis_length, 0.0f, -0.03*axis_length);
	}
	glEnd();

    glColor3f(0.0f, 1.0f, 0.0f);//指定线的颜色,绿色
	glBegin(GL_LINES);
	{
		//axis_length=-axis_length;// 解决y坐标反向问题
		// y-axis
		glVertex3f(0.0f, 0.0f, 0.0f);
		glVertex3f(0.0f, axis_length, 0.0f);

		// y-axis arrow
		glVertex3f(0.0f, axis_length, 0.0f);
		glVertex3f(0.03*axis_length, 0.95*axis_length, 0.0f);
		glVertex3f(0.0f, axis_length, 0.0f);
		glVertex3f(-0.03*axis_length, 0.95*axis_length, 0.0f);
		glVertex3f(0.0f, axis_length, 0.0f);
		glVertex3f(0.0f, 0.95*axis_length, 0.03*axis_length);
		glVertex3f(0.0f, axis_length, 0.0f);
		glVertex3f(0.0f, 0.95*axis_length, -0.03*axis_length);
	}
	glEnd();

    glColor3f(0.0f, 0.0f, 1.0f);//指定线的颜色,蓝色
	glBegin(GL_LINES);
	{
		// z-axis
		glVertex3f(0.0f, 0.0f, 0.0f);
		glVertex3f(0.0f, 0.0f, axis_length);

		// z-axis arrow
		glVertex3f(0.0f, 0.0f, axis_length);
		glVertex3f(0.0f, 0.03*axis_length, 0.95*axis_length);
		glVertex3f(0.0f, 0.0f, axis_length);
		glVertex3f(0.0f, -0.03*axis_length, 0.95*axis_length);
		glVertex3f(0.0f, 0.0f, axis_length);
		glVertex3f(0.03*axis_length, 0.0f, 0.95*axis_length);
		glVertex3f(0.0f, 0.0f, axis_length);
		glVertex3f(-0.03*axis_length, 0.0f, 0.95*axis_length);
	}
	glEnd();
}

// 绘制磁芯
void DrawMagCore(GLfloat Rin, GLfloat Rout, GLfloat H, GLfloat Gap)
{
    glEnable(GL_POLYGON_OFFSET_FILL);// 解决线框遮挡
    glPolygonOffset(1.0f, 1.0f);// 解决线框遮挡

    int i;
    float x_start,y_start,x_now,y_now,start_angle,stop_angle,start_angle_2,stop_angle_2;

    // 画面
    glColor3f(0.5f, 0.5f, 0.5f);
    glBegin(GL_QUADS);
    {
        glVertex3f(-Gap/2, -sqrt(Rin*Rin-Gap*Gap/4), -H/2);
        glVertex3f(-Gap/2, -sqrt(Rin*Rin-Gap*Gap/4),  H/2);
        glVertex3f(-Gap/2, -sqrt(Rout*Rout-Gap*Gap/4),  H/2);
        glVertex3f(-Gap/2, -sqrt(Rout*Rout-Gap*Gap/4), -H/2);
    }
    glEnd();
    glBegin(GL_QUADS);
    {
        glVertex3f( Gap/2, -sqrt(Rin*Rin-Gap*Gap/4), -H/2);
        glVertex3f( Gap/2, -sqrt(Rin*Rin-Gap*Gap/4),  H/2);
        glVertex3f( Gap/2, -sqrt(Rout*Rout-Gap*Gap/4),  H/2);
        glVertex3f( Gap/2, -sqrt(Rout*Rout-Gap*Gap/4), -H/2);
    }
    glEnd();
    // 外柱面
    start_angle=-M_PI/2+asin(Gap/2/Rout);
    stop_angle=3*M_PI/2-asin(Gap/2/Rout);
    float x_1,y_1,x_2,y_2;
    for (i = 0; i < 100; i++) {
        x_1=Rout*cos(start_angle+(float)i*(stop_angle-start_angle)/100);
        y_1=Rout*sin(start_angle+(float)i*(stop_angle-start_angle)/100);
        x_2=Rout*cos(start_angle+(float)(i+1)*(stop_angle-start_angle)/100);
        y_2=Rout*sin(start_angle+(float)(i+1)*(stop_angle-start_angle)/100);
        glBegin(GL_POLYGON);
        {
            glVertex3f(x_1, y_1, -H/2);
            glVertex3f(x_1, y_1,  H/2);
            glVertex3f(x_2, y_2,  H/2);
            glVertex3f(x_2, y_2, -H/2);
        }
        glEnd();
    }
    // 内柱面
    start_angle_2=-M_PI/2+asin(Gap/2/Rin);
    stop_angle_2=3*M_PI/2-asin(Gap/2/Rin);
    for (i = 0; i < 100; i++) {
        x_1=Rin*cos(start_angle_2+(float)i*(stop_angle_2-start_angle_2)/100);
        y_1=Rin*sin(start_angle_2+(float)i*(stop_angle_2-start_angle_2)/100);
        x_2=Rin*cos(start_angle_2+(float)(i+1)*(stop_angle_2-start_angle_2)/100);
        y_2=Rin*sin(start_angle_2+(float)(i+1)*(stop_angle_2-start_angle_2)/100);
        glBegin(GL_POLYGON);
        {
            glVertex3f(x_1, y_1, -H/2);
            glVertex3f(x_1, y_1,  H/2);
            glVertex3f(x_2, y_2,  H/2);
            glVertex3f(x_2, y_2, -H/2);
        }
        glEnd();
    }
    // 上面
    float x_3,y_3,x_4,y_4;
    for (i = 0; i < 100; i++) {
        x_1=Rin*cos(start_angle_2+(float)i*(stop_angle_2-start_angle_2)/100);
        y_1=Rin*sin(start_angle_2+(float)i*(stop_angle_2-start_angle_2)/100);
        x_2=Rin*cos(start_angle_2+(float)(i+1)*(stop_angle_2-start_angle_2)/100);
        y_2=Rin*sin(start_angle_2+(float)(i+1)*(stop_angle_2-start_angle_2)/100);
        x_3=Rout*cos(start_angle+(float)i*(stop_angle-start_angle)/100);
        y_3=Rout*sin(start_angle+(float)i*(stop_angle-start_angle)/100);
        x_4=Rout*cos(start_angle+(float)(i+1)*(stop_angle-start_angle)/100);
        y_4=Rout*sin(start_angle+(float)(i+1)*(stop_angle-start_angle)/100);
        glBegin(GL_POLYGON);
        {
            glVertex3f(x_1, y_1,  H/2);
            glVertex3f(x_2, y_2,  H/2);
            glVertex3f(x_4, y_4,  H/2);
            glVertex3f(x_3, y_3,  H/2);
        }
        glEnd();
    }
    // 下面
    for (i = 0; i < 100; i++) {
        x_1=Rin*cos(start_angle_2+(float)i*(stop_angle_2-start_angle_2)/100);
        y_1=Rin*sin(start_angle_2+(float)i*(stop_angle_2-start_angle_2)/100);
        x_2=Rin*cos(start_angle_2+(float)(i+1)*(stop_angle_2-start_angle_2)/100);
        y_2=Rin*sin(start_angle_2+(float)(i+1)*(stop_angle_2-start_angle_2)/100);
        x_3=Rout*cos(start_angle+(float)i*(stop_angle-start_angle)/100);
        y_3=Rout*sin(start_angle+(float)i*(stop_angle-start_angle)/100);
        x_4=Rout*cos(start_angle+(float)(i+1)*(stop_angle-start_angle)/100);
        y_4=Rout*sin(start_angle+(float)(i+1)*(stop_angle-start_angle)/100);
        glBegin(GL_POLYGON);
        {
            glVertex3f(x_1, y_1, -H/2);
            glVertex3f(x_2, y_2, -H/2);
            glVertex3f(x_4, y_4, -H/2);
            glVertex3f(x_3, y_3, -H/2);
        }
        glEnd();
    }
    // 画线
    glColor3f(0.0f, 0.0f, 0.0f);//指定线的颜色
	glBegin(GL_LINES);
	{
        glVertex3f(-Gap/2, -sqrt(Rin*Rin-Gap*Gap/4), -H/2);
        glVertex3f(-Gap/2, -sqrt(Rin*Rin-Gap*Gap/4),  H/2);
	}
	glEnd();
    glBegin(GL_LINES);
    {
        glVertex3f( Gap/2, -sqrt(Rin*Rin-Gap*Gap/4), -H/2);
        glVertex3f( Gap/2, -sqrt(Rin*Rin-Gap*Gap/4),  H/2);
    }
    glEnd();
    glBegin(GL_LINES);
    {
        glVertex3f(-Gap/2, -sqrt(Rout*Rout-Gap*Gap/4), -H/2);
        glVertex3f(-Gap/2, -sqrt(Rout*Rout-Gap*Gap/4),  H/2);
    }
    glEnd();
    glBegin(GL_LINES);
    {
        glVertex3f( Gap/2, -sqrt(Rout*Rout-Gap*Gap/4), -H/2);
        glVertex3f( Gap/2, -sqrt(Rout*Rout-Gap*Gap/4),  H/2);
    }
    glEnd();
    glBegin(GL_LINES);
    {
        glVertex3f(-Gap/2, -sqrt(Rin*Rin-Gap*Gap/4), -H/2);
        glVertex3f(-Gap/2, -sqrt(Rout*Rout-Gap*Gap/4), -H/2);
    }
    glEnd();
    glBegin(GL_LINES);
    {
        glVertex3f( Gap/2, -sqrt(Rin*Rin-Gap*Gap/4), -H/2);
        glVertex3f( Gap/2, -sqrt(Rout*Rout-Gap*Gap/4), -H/2);
    }
    glEnd();
    glBegin(GL_LINES);
    {
        glVertex3f(-Gap/2, -sqrt(Rin*Rin-Gap*Gap/4),  H/2);
        glVertex3f(-Gap/2, -sqrt(Rout*Rout-Gap*Gap/4),  H/2);
    }
    glEnd();
    glBegin(GL_LINES);
    {
        glVertex3f( Gap/2, -sqrt(Rin*Rin-Gap*Gap/4),  H/2);
        glVertex3f( Gap/2, -sqrt(Rout*Rout-Gap*Gap/4),  H/2);
    }
    glEnd();

    start_angle=-M_PI/2+asin(Gap/2/Rin);
    stop_angle=3*M_PI/2-asin(Gap/2/Rin);
    x_start=Rin*cos(start_angle);
    y_start=Rin*sin(start_angle);
    glBegin(GL_LINE_STRIP);
    {
        glVertex3f( x_start, y_start, -H/2);
        for (i = 0; i < 100; i++) {
            x_now=Rin*cos(start_angle+(float)(i+1)*(stop_angle-start_angle)/100.0);
            y_now=Rin*sin(start_angle+(float)(i+1)*(stop_angle-start_angle)/100.0);
            glVertex3f( x_now, y_now, -H/2);
        }
    }
    glEnd();
    glBegin(GL_LINE_STRIP);
    {
        glVertex3f( x_start, y_start, H/2);
        for (i = 0; i < 100; i++) {
            x_now=Rin*cos(start_angle+(float)(i+1)*(stop_angle-start_angle)/100.0);
            y_now=Rin*sin(start_angle+(float)(i+1)*(stop_angle-start_angle)/100.0);
            glVertex3f( x_now, y_now, H/2);
        }
    }
    glEnd();
    start_angle=-M_PI/2+asin(Gap/2/Rout);
    stop_angle=3*M_PI/2-asin(Gap/2/Rout);
    x_start=Rout*cos(start_angle);
    y_start=Rout*sin(start_angle);
    glBegin(GL_LINE_STRIP);
    {
        glVertex3f( x_start, y_start, -H/2);
        for (i = 0; i < 100; i++) {
            x_now=Rout*cos(start_angle+(float)(i+1)*(stop_angle-start_angle)/100.0);
            y_now=Rout*sin(start_angle+(float)(i+1)*(stop_angle-start_angle)/100.0);
            glVertex3f( x_now, y_now, -H/2);
        }
    }
    glEnd();
    glBegin(GL_LINE_STRIP);
    {
        glVertex3f( x_start, y_start,  H/2);
        for (i = 0; i < 100; i++) {
            x_now=Rout*cos(start_angle+(float)(i+1)*(stop_angle-start_angle)/100.0);
            y_now=Rout*sin(start_angle+(float)(i+1)*(stop_angle-start_angle)/100.0);
            glVertex3f( x_now, y_now,  H/2);
        }
    }
    glEnd();

    glDisable(GL_POLYGON_OFFSET_FILL);// 解决线框遮挡
}

// 绘图参数设置
void SetupRC()
{
    glClearColor(0.5,0.7,0.9,0.0); //指定背景颜色（依次为RGBA）
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    glFlush();
    //glutSwapBuffers();
	glShadeModel(GL_SMOOTH);
	glEnable(GL_DEPTH_TEST);//被遮住的部分不绘制

	glEnable(GL_POINT_SMOOTH);
	glEnable(GL_LINE_SMOOTH);
	glEnable(GL_POLYGON_SMOOTH);
	glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
	glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
	glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
}


