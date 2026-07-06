#ifndef BASEMOB_H
#define BASEMOB_H

#include<QPainter>
#include<QGraphicsItem>
#include<QGraphicsScene>
#include<QGraphicsObject>
#include<QPointer>
#include<QMovie>
#include<QList>
#include<QPointF>
#include<QString>

class Carrot;

struct Checkpoint {
    QPointF pos;
    double waitTime = 0; // 等待时间（秒），0 表示不等待
};

class BaseMob : public QGraphicsObject
{
    Q_OBJECT
public:
    int hp;
    int maxHp = 1;
    int state;
    qreal speed;
    bool reachedEnd = false;
    int taunt = 0; // 嘲讽值，炮塔优先攻击高嘲讽单位
    int mobType = 1; // 敌人种类编号（1=soldier, 2=dog, 3=tank），用于击杀金币
    QList<Checkpoint> checkpoints;
    int cpIndex = 0;
    static Carrot* carrot;
    enum{Type = UserType + 2};
    BaseMob();
    ~BaseMob();
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    int type() const override;
    void setMovie(QString path);
    void setIdleMovie(QString path);
    void setIdleBeginMovie(QString path);
    void setIdleEndMovie(QString path);
    double remainingDistance() const;
    void advance(int phase) override;

protected:
    QMovie *movie;
    QMovie *idleMovie = nullptr;
    QString moveMoviePath;
    QString idleMoviePath;
    QString idleBeginPath;
    QString idleEndPath;
    bool facingLeft = false;
    int imageOffsetY = 0;
    bool isWaiting = false;
    int waitTicks = 0;
    int waitTicksNeeded = 0;
    int animTicks = 0;
    int animEndTicks = 0;
    enum WaitPhase { WaitNone, WaitIdleBegin, WaitIdleLoop, WaitIdleEnd };
    WaitPhase waitPhase = WaitNone;

    void switchToIdleBegin();
    void switchToIdleLoop();
    void switchToIdleEnd();
    void switchToMove();
    void stopAndDeleteMovie();
};


#endif // BASEMOB_H
