#ifndef SHELL_H
#define SHELL_H

#include "other.h"
#include "basemob.h"

class Shell : public Other
{
public:
    Shell(int attack = 0,BaseMob *target = nullptr,bool flag = false);
    QRectF boundingRect() const override;
    void paint(QPainter *painter,const QStyleOptionGraphicsItem *option,QWidget *widget) override;
    bool collidesWithItem(const QGraphicsItem *other,Qt::ItemSelectionMode mode) const override;
    void advance(int phase) override;

private:
    int atk;    //子弹攻击力
    qreal speed;    //子弹速度
    qreal last_x,last_y; //最后一次目的地
    QPointer<BaseMob> target;
};

#endif // SHELL_H
