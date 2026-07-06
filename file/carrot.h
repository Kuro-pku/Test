#ifndef CARROT_H
#define CARROT_H

#include <QGraphicsItem>
#include <QPainter>
#include <QPixmap>

class Carrot : public QGraphicsItem
{
public:
    Carrot();
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    void takeDamage(int dmg);
    bool isDead() const { return hp <= 0; }
    int hp = 10;

private:
    QPixmap fullPixmap, midPixmap, lowPixmap;
    const QPixmap &currentPixmap() const;
};

#endif // CARROT_H
