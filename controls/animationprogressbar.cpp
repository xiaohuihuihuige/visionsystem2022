#include "animationprogressbar.h"
#include <QPainter>
#include<QFont>

AnimationProgressbar::AnimationProgressbar(QWidget *parent) : QWidget(parent),
    m_animaindex(0),
    m_animaTotal(30),
    m_persent(0)
{
    QPixmap aniMap(":/image/image/sequence_animate.png");

    for(int i=0; i<m_animaTotal; i++){
        m_animalist << aniMap.copy(i*(aniMap.width()/m_animaTotal), 0, aniMap.width()/m_animaTotal, aniMap.height());
    }

    m_animation=new QPropertyAnimation(this,"");
    m_animation->setStartValue(0);
    m_animation->setEndValue(m_animaTotal - 1);
    m_animation->setDuration(1000);
    m_animation->setLoopCount(-1);
    connect(m_animation,&QPropertyAnimation::valueChanged,this,&AnimationProgressbar::slot_valuechange);
    startAnimation();
}

void AnimationProgressbar::startAnimation()
{
    update();
    m_animation->start();
}
void AnimationProgressbar::setPersent(int precent)
{
    precentText=QString("%1%").arg(QString::number(precent));
}

void AnimationProgressbar::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::SmoothPixmapTransform);

    int side = qMin(width(), height());
    QRect outRect(0, 0, side, side);
    QRect inRect(20, 20, side-40, side-40);

    QString valueStr = QString("%1%").arg(QString::number(m_persent));

    //画圆环
    p.drawPixmap(outRect, m_animalist.at(m_animaindex));

    //画文字
    QFont f = QFont("Microsoft YaHei", 15, QFont::Bold);
    p.setFont(f);
    p.setPen(QColor("#555555"));
    p.drawText(inRect, Qt::AlignCenter, precentText);
}

void AnimationProgressbar::slot_valuechange(QVariant var)
{
    m_animaindex = var.toInt();
    //m_persent = m_animaindex*100 / m_animaTotal;
    update();
}
