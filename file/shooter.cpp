#include "shooter.h"
#include <QtMath>

Shooter::Shooter()
{
    atk = 15;
    time = int(1.0 * 1000 / 32 / 1.5);
    totalCost = 100;
    setMovie(":/new/prefix1/images/exusiai_shooter.gif");
}

Shooter::~Shooter()
{
    delete movie;
    movie = nullptr;
}

QRectF Shooter::boundingRect() const
{
    return QRectF(-62.5, -62.5, 125, 125);
}

void Shooter::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option)
    Q_UNUSED(widget)

    painter->save();
    if (facingLeft) {
        painter->scale(-1, 1);
    }
    painter->drawImage(boundingRect(), movie->currentImage());
    painter->restore();

    if (level >= 1 && level <= 3) {
        static const QColor colors[] = { QColor(70, 180, 70), QColor(255, 140, 0), QColor(210, 50, 50) };
        painter->save();
        QRectF r = boundingRect();
        int cx = r.left() + 34;
        int cy = r.bottom() - 28;
        painter->setBrush(colors[level - 1]);
        painter->setPen(QPen(Qt::white, 1.5));
        painter->drawEllipse(QPoint(cx, cy), 9, 9);
        painter->setPen(Qt::white);
        painter->setFont(QFont("Arial", 10, QFont::Bold));
        painter->drawText(QRectF(cx - 9, cy - 9, 18, 18), Qt::AlignCenter, QString::number(level));
        painter->restore();
    }
}

int Shooter::getUpgradeCost() const {
    if (level == 1) return 180;
    if (level == 2) return 260;
    return 0;
}

bool Shooter::upgrade()
{
    if (level >= 3) return false;
    level++;
    atk *= 1.5;
    time /= 1.5;
    rangePx *= 1.2;
    totalCost += getUpgradeCost();
    return true;
}

bool Shooter::collidesWithItem(const QGraphicsItem *other, Qt::ItemSelectionMode mode) const
{
    Q_UNUSED(mode)
    return other->type() == BaseMob::Type;
}

void Shooter::advance(int phase)
{
    if(!phase) return;
    update();
    QList<QGraphicsItem *> items = collidingItems();
    if(!items.isEmpty()){
        // 优先攻击嘲讽值最高的单位；嘲讽值相同时选离终点最近的
        BaseMob *target = nullptr;
        int bestTaunt = -1;
        double bestRemaining = 1e9;
        for (auto *it : items) {
            BaseMob *m = qgraphicsitem_cast<BaseMob *>(it);
            if (!m) continue;
            double dx = m->x() - x();
            double dy = m->y() - y();
            double dist = qSqrt(dx * dx + dy * dy);
            if (dist > rangePx) continue;
            double remaining = m->remainingDistance();
            if (m->taunt > bestTaunt || (m->taunt == bestTaunt && remaining < bestRemaining)) {
                bestTaunt = m->taunt;
                bestRemaining = remaining;
                target = m;
            }
        }
        if (target) {
            facingLeft = target->x() < x();
            if (++counter >= time) {
                counter = 0;
                Shell *shell = new Shell(atk, target);
                shell->setX(x() + (facingLeft ? -32 : 32));
                shell->setY(y());
                shell->setZValue(2.5);
                scene()->addItem(shell);
            }
        }
    }
}
