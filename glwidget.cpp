#include "glwidget.h"

// ========== 初始化轨迹球操作矩阵 ==========
Matrix4fT   Transform   = {  1.0f,  0.0f,  0.0f,  0.0f,
0.0f,  1.0f,  0.0f,  0.0f,
0.0f,  0.0f,  1.0f,  0.0f,
0.0f,  0.0f,  0.0f,  1.0f };

Matrix3fT   LastRot     = {  1.0f,  0.0f,  0.0f,
0.0f,  1.0f,  0.0f,
0.0f,  0.0f,  1.0f };

Matrix3fT   ThisRot     = {  1.0f,  0.0f,  0.0f,
0.0f,  1.0f,  0.0f,
0.0f,  0.0f,  1.0f };

ArcBallT    ArcBall(2.0f, 2.0f);
ArcBallT*    arcBall =&ArcBall;
Point2fT    MousePt;
// ==========================================

GLWidget::GLWidget(QWidget *parent)
    : QOpenGLWidget(parent),
    plot(false),
    m_xMov(0),
    m_yMov(0),
    m_zMov(0),
    m_scale(1)
{

}

void GLWidget::initializeGL()
{
    SetupRC();
}

void GLWidget::paintGL()
{
    if (plot) {
        int width=this->width();
        int height=this->height();
        glViewport(0,0,width,height);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity(); // 重置为单位矩阵

        // 平移
        glTranslatef(m_xMov, m_yMov, m_zMov);
        // 旋转
        glMultMatrixf(Transform.M);
        // 缩放
        glScalef(m_scale, m_scale, m_scale);

        // 计算缩放
        double smax=2*Rout;

        // 画坐标轴
        //if (show_axis) {
            DrawAxis(0.3f*Rin/Rout);
       // }

        // 画磁铁
        DrawMagCore(Rin/smax, Rout/smax, H/smax, Gap/smax);
    }
}

void GLWidget::resizeGL(int w, int h)
{
    arcBall->setBounds((GLfloat)w, (GLfloat)h);//设置窗口边界，解决缩放后旋转问题
    glViewport(0,0,(GLsizei) w, (GLsizei) h);
    float nscale=1;

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    if (w<h) {
        glOrtho(-nscale,nscale,-nscale*h/w,nscale*h/w,-4*nscale,4*nscale);
    } else {
        glOrtho(-nscale*w/h,nscale*w/h,-nscale,nscale,-4*nscale,4*nscale);
    }

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void GLWidget::mousePressEvent(QMouseEvent *event)
{
    if (plot) {
        m_lastPos = event->pos();
        m_pressPos = event->pos();

        MousePt.s.X = m_lastPos.x();
        MousePt.s.Y = m_lastPos.y();
        LastRot = ThisRot;
        ArcBall.click(&MousePt);
    }
}

void GLWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (plot) {
        int dx = event->x() - m_lastPos.x();
        int dy = event->y() - m_lastPos.y();

        // 鼠标左键拖曳旋转
        if (event->buttons() & Qt::LeftButton) {
            MousePt.s.X = event->pos().x();
            MousePt.s.Y = event->pos().y();
            Quat4fT ThisQuat;

            ArcBall.drag(&MousePt, &ThisQuat);                        // Update End Vector And Get Rotation As Quaternion
            Matrix3fSetRotationFromQuat4f(&ThisRot, &ThisQuat);        // Convert Quaternion Into Matrix3fT
            Matrix3fMulMatrix3f(&ThisRot, &LastRot);                // Accumulate Last Rotation Into This One
            Matrix4fSetRotationFromMatrix3f(&Transform, &ThisRot);    // Set Our Final Transform's Rotation From This One
        }

        // 鼠标右键拖曳移动
        if (event->buttons() & Qt::RightButton) {
            m_xMov=m_xMov + (float)dx/qMin(width(),height())*2;
            m_yMov=m_yMov - (float)dy/qMin(width(),height())*2;
        }

        update();
        m_lastPos = event->pos();

        // 当前的模型矩阵
        GLdouble modelMatrix[16];
        glGetDoublev( GL_MODELVIEW_MATRIX , modelMatrix );
        // 当前的投影矩阵
        GLdouble projMatrix[16];
        glGetDoublev( GL_PROJECTION_MATRIX , projMatrix );
        // 当前的视口信息
        GLint viewport[4];
        glGetIntegerv( GL_VIEWPORT , viewport );
    }
}

void GLWidget::wheelEvent(QWheelEvent *event)
{
    if (plot) {
        // 鼠标滚轮缩放大小
        if (event->delta() > 0)
        {
            m_scale+=0.1;
        } else {
            m_scale-=0.1;
        }
        if (m_scale < 0.1) {
            m_scale=0.1;
        }

        update();
    }
}
