#include "ground.h"

ground::ground(int st)
{
    state = st;
    grassPng = QPixmap(":/new/prefix1/images/moss_block.png");
    if(st == 1)
    {
        png = QPixmap(":/new/prefix1/images/moss_block.png");
    }
    else if(st == 2)
    {
        png = QPixmap(":/new/prefix1/images/dirt_path_top.png");
    }
}

QRectF ground::boundingRect() const
{
    return QRectF( -40, -40, 80, 80);
}

void ground::paint(QPainter *painter,const QStyleOptionGraphicsItem *option,QWidget *widget)
{
    Q_UNUSED(option)
    Q_UNUSED(widget)
    if (state == 2) {
        painter->drawPixmap(QRect(-40, -40, 80, 80), grassPng);
        painter->drawPixmap(QRect(-32, -32, 64, 64), png);
        return;
    }
    painter->drawPixmap(QRect(-40, -40, 80, 80), png);
};

void ground::advance(int phase)
{

}

// void ground::mousePressEvent(QGraphicsSceneMouseEvent *event)
// {
//     if (state == 1&&event->button() == Qt::LeftButton && !hasShooter) {
//         hasShooter = true;
//         Shooter *shooter = new Shooter();
//         shooter->setPos(x(), y()-20);  // 根据实际调整偏移
//         scene()->addItem(shooter);
//         qDebug() << "Shooter placed at" << pos();
//     }
// }

bool ground::invalid()
{
    return state!=1 || hasShooter;
}
