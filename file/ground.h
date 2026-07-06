#ifndef GROUND_H
#define GROUND_H


#include "other.h"
#include "shooter.h"

class ground : public Other
{
public:
    ground(int st);
    QRectF boundingRect() const override;
    void paint(QPainter *painter,const QStyleOptionGraphicsItem *option,QWidget *widget) override;
    void advance(int phase) override;
    //void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void setShooterPlaced() { hasShooter = true; }
    void clearShooterPlaced() { hasShooter = false; }
    bool hasShooterPlaced() const { return hasShooter; }
    bool invalid();

private:
    int state;
    QPixmap png;
    QPixmap grassPng;
    bool hasShooter = false;
};

#endif // GROUND_H
