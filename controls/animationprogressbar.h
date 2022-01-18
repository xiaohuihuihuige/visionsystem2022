#ifndef ANIMATIONPROGRESSBAR_H
#define ANIMATIONPROGRESSBAR_H

#include <QWidget>
#include <QPropertyAnimation>

class AnimationProgressbar : public QWidget
{
    Q_OBJECT

public:
    AnimationProgressbar(QWidget *parent = nullptr);

    void startAnimation();

    void setPersent(int precent);
protected:
    void paintEvent(QPaintEvent*);

private:
    QList<QPixmap> m_animalist;
    QPropertyAnimation *m_animation;
    int m_animaindex;
    int m_animaTotal;
    int m_persent;
    QString precentText;

private slots:
    void slot_valuechange(QVariant var);
};

#endif // ANIMATIONPROGRESSBAR_H
