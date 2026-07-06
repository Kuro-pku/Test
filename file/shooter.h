#ifndef SHOOTER_H
#define SHOOTER_H

#include "BaseBattery.h"
#include "shell.h"
#include "basemob.h"
#include <QMovie>

class Shooter : public BaseBattery
{
public:
    Shooter();
    ~Shooter() override;
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    void advance(int phase) override;
    bool collidesWithItem(const QGraphicsItem *other, Qt::ItemSelectionMode mode) const override;
    bool upgrade();
    int getUpgradeCost() const;
    int getTotalCost() const { return totalCost; }

    double rangePx = 160.0;

private:
    int totalCost = 0;
    bool facingLeft = false;
};

#endif // SHOOTER_H
