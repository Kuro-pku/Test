#include "shell.h"

Shell::Shell(int attack, BaseMob* _target,bool flag)
{
    atk = attack;
    target = _target;
    last_x = _target->x();
    last_y = _target->y();
    speed = 660.0 * 32 / 1000;
}

QRectF Shell::boundingRect() const
{
    return QRectF(-12,-28,24,24);
}

void Shell::paint(QPainter *painter,const QStyleOptionGraphicsItem *option,QWidget *widget)
{
    Q_UNUSED(option)
    Q_UNUSED(widget)
    painter->drawPixmap(QRect(-12,-28,24,24), QPixmap(":/new/prefix1/images/Pea.png"));
}

bool Shell::collidesWithItem(const QGraphicsItem *other,Qt::ItemSelectionMode mode) const
{
    Q_UNUSED(mode)
    return other->type() == BaseMob::Type && qAbs(other->y()- y()) < 15 && qAbs(other->x() - x()) < 15;
}

void Shell::advance(int phase)
{
    if(!phase) return;
    update();

    if (!target.isNull()) {
        last_x = target->x();
        last_y = target->y();
    }

    if (qAbs(last_y - y()) < 15 && qAbs(last_x - x()) < 15) {
        if(!target.isNull()) target->hp -= atk;
        delete this;
        return;
    }
    qreal dy = last_y-y();
    qreal dx = last_x-x();
    setX(x() + dx/(qAbs(dx)+qAbs(dy)+1e-6) * speed );
    setY(y() + dy/(qAbs(dx)+qAbs(dy)+1e-6) * speed );

    if(x() > 1369 || y() > 1069) delete this;
}