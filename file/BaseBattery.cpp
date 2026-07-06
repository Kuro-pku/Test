#include "BaseBattery.h"

BaseBattery::BaseBattery()
{
    movie = nullptr;
    atk = counter = state = time = 0;
    level = 1;
}

BaseBattery::~BaseBattery()
{
    delete movie;
}

QRectF BaseBattery::boundingRect() const
{
    return QRectF(-50,-50,100,100);
}

void BaseBattery::paint(QPainter *painter,const QStyleOptionGraphicsItem *option,QWidget *widget)
{
    Q_UNUSED(option)
    Q_UNUSED(widget)
    painter->drawImage(boundingRect(),movie->currentImage());

    // 左下角等级数字（绿→橙→红）
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

int BaseBattery::type()
{
    return Type;
}

void BaseBattery::setMovie(QString path)
{
    if(movie) delete movie;
    movie = new QMovie(path);
    movie->start();
}
