#include "carrot.h"

Carrot::Carrot()
{
    fullPixmap  = QPixmap(":/new/prefix1/images/carrot_full.png");
    midPixmap   = QPixmap(":/new/prefix1/images/carrot_mid.png");
    lowPixmap   = QPixmap(":/new/prefix1/images/carrot_low.png");
}

const QPixmap &Carrot::currentPixmap() const
{
    if (hp <= 3)  return lowPixmap;
    if (hp <= 9)  return midPixmap;
    return fullPixmap;
}

QRectF Carrot::boundingRect() const
{
    return QRectF(-60, -60, 130, 130);
}

void Carrot::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    if (hp <= 0) return;
    const QPixmap &img = currentPixmap();
    painter->drawPixmap(QPointF(-img.width() / 2.0, -img.height() / 2.0), img);

    if (hp > 0 && hp < 10) {
        int cx = 32;
        int cy = -32;
        painter->save();
        painter->setBrush(Qt::red);
        painter->setPen(QPen(Qt::black, 1.5));
        painter->drawEllipse(QPointF(cx, cy), 13, 13);
        painter->setPen(QColor(100, 180, 255));
        painter->setFont(QFont("Arial", 9, QFont::Bold));
        QString text = QChar(0x2665) + QString::number(hp);
        painter->drawText(QRectF(cx - 13, cy - 13, 26, 26), Qt::AlignCenter, text);
        painter->restore();
    }
}

void Carrot::takeDamage(int dmg)
{
    hp -= dmg;
    if (hp < 0) hp = 0;
    if (hp <= 0) hide();
    update();
}
