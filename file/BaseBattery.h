#ifndef BASEBATTERY_H
#define BASEBATTERY_H

#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QPainter>
#include <QMovie>


class BaseBattery : public QGraphicsItem
{
public:
    int state;
    int level;
    enum{Type = UserType + 1};
    BaseBattery();
    ~BaseBattery();
    QRectF boundingRect() const override;
    void paint(QPainter *painter,const QStyleOptionGraphicsItem *option,QWidget *widget) override;
    int type();
    void setMovie(QString path);


protected:
    QMovie *movie;
    int atk;
    int counter;
    int time;

};


#endif // BASEBATTERY_H
