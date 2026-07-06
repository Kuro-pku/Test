#include "basemob.h"
#include "carrot.h"
#include <QPainterPath>
#include <QtMath>

Carrot* BaseMob::carrot = nullptr;

BaseMob::BaseMob()
{
    movie = nullptr;
    hp = state = 0;
    speed = 0.0;
}

BaseMob::~BaseMob()
{
    // movie 和 idleMovie 可能指向同一对象，避免 double delete
    stopAndDeleteMovie();
}

int BaseMob::type() const
{
    return Type;
}

QRectF BaseMob::boundingRect() const
{
    return QRect(-80,-100,200,140);
}

void BaseMob::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option)
    Q_UNUSED(widget)
    if (!movie) return;
    painter->save();
    if (facingLeft)
        painter->scale(-1, 1);
    painter->drawImage(QRect(-70, -100 + imageOffsetY, 140, 140), movie->currentImage());
    painter->restore();

    if (hp > 0 && maxHp > 0) {
        int barW = 30, barH = 4;
        int barX = -20, barY = -83;
        double ratio = qMin(1.0, (double)hp / maxHp);
        painter->save();
        painter->setBrush(QColor(40, 40, 40));
        painter->setPen(Qt::NoPen);
        QPainterPath bgPath;
        bgPath.addRoundedRect(QRectF(barX, barY, barW, barH), 2, 2);
        painter->drawPath(bgPath);
        painter->setBrush(QColor(255, 165, 0));
        QPainterPath fillPath;
        fillPath.addRoundedRect(QRectF(barX, barY, int(barW * ratio), barH), 2, 2);
        painter->drawPath(fillPath);
        painter->restore();
    }
}

void BaseMob::setMovie(QString path)
{
    moveMoviePath = path;
    if (movie)
        delete movie;
    movie = new QMovie(path);
    movie->start();
}

void BaseMob::setIdleMovie(QString path)
{
    idleMoviePath = path;
}

void BaseMob::setIdleBeginMovie(QString path)
{
    idleBeginPath = path;
}

void BaseMob::setIdleEndMovie(QString path)
{
    idleEndPath = path;
}

void BaseMob::stopAndDeleteMovie()
{
    if (movie) {
        movie->stop();
        // idleMovie 和 movie 可能指向同一对象
        if (idleMovie == movie)
            idleMovie = nullptr;
        delete movie;
        movie = nullptr;
    }
    if (idleMovie) {
        delete idleMovie;
        idleMovie = nullptr;
    }
}

void BaseMob::switchToIdleBegin()
{
    if (idleBeginPath.isEmpty()) {
        // 没有 begin 动画，直接进入 idle loop
        switchToIdleLoop();
        return;
    }
    stopAndDeleteMovie();
    idleMovie = new QMovie(idleBeginPath);
    idleMovie->setCacheMode(QMovie::CacheAll);
    movie = idleMovie;
    movie->start();
    // 计算动画总 tick 数（帧数 × 帧间隔 / 32ms）
    int fc = movie->frameCount();
    int delay = qMax(movie->nextFrameDelay(), 30);
    animEndTicks = qMax(1, fc * delay / 32);
    animTicks = 0;
}

void BaseMob::switchToIdleLoop()
{
    if (idleMoviePath.isEmpty()) {
        if (movie) { movie->start(); }
        return;
    }
    stopAndDeleteMovie();
    idleMovie = new QMovie(idleMoviePath);
    movie = idleMovie;
    movie->start();
}

void BaseMob::switchToIdleEnd()
{
    if (idleEndPath.isEmpty()) {
        // 没有 end 动画，直接进入 move
        switchToMove();
        return;
    }
    stopAndDeleteMovie();
    idleMovie = new QMovie(idleEndPath);
    idleMovie->setCacheMode(QMovie::CacheAll);
    movie = idleMovie;
    movie->start();
    // 计算动画总 tick 数
    int fc = movie->frameCount();
    int delay = qMax(movie->nextFrameDelay(), 30);
    animEndTicks = qMax(1, fc * delay / 32);
    animTicks = 0;
}

void BaseMob::switchToMove()
{
    stopAndDeleteMovie();
    movie = new QMovie(moveMoviePath);
    movie->start();
}

double BaseMob::remainingDistance() const
{
    if (cpIndex >= checkpoints.size()) return 0;
    double dist = 0;
    QPointF cur(x(), y());
    for (int i = cpIndex; i < checkpoints.size(); i++) {
        dist += qAbs(checkpoints[i].pos.x() - cur.x()) + qAbs(checkpoints[i].pos.y() - cur.y());
        cur = checkpoints[i].pos;
    }
    return dist;
}

void BaseMob::advance(int phase)
{
    if (!phase) return;
    if (hp <= 0) return;

    update();

    if (isWaiting) {
        switch (waitPhase) {
        case WaitIdleBegin:
            animTicks++;
            if (animTicks >= animEndTicks) {
                switchToIdleLoop();
                waitPhase = WaitIdleLoop;
            }
            return;

        case WaitIdleLoop:
            waitTicks++;
            if (waitTicks >= waitTicksNeeded) {
                switchToIdleEnd();
                waitPhase = WaitIdleEnd;
                waitTicks = 0;
            }
            return;

        case WaitIdleEnd:
            animTicks++;
            if (animTicks >= animEndTicks) {
                isWaiting = false;
                switchToMove();
                waitPhase = WaitNone;
                cpIndex++;
                if (cpIndex >= checkpoints.size()) {
                    if (carrot)
                        carrot->takeDamage(1);
                    reachedEnd = true;
                    hp = 0;
                    return;
                }
            }
            return;
        }
        return;
    }

    if (cpIndex >= checkpoints.size()) return;

    Checkpoint &cp = checkpoints[cpIndex];
    QPointF target = cp.pos;
    qreal dx = target.x() - x();
    qreal dy = target.y() - y();

    if (qAbs(dx) > 0.5) {
        facingLeft = dx < 0;
        qreal step = qMin(speed, qAbs(dx));
        setX(x() + (dx > 0 ? step : -step));
    } else if (qAbs(dy) > 0.5) {
        qreal step = qMin(speed, qAbs(dy));
        setY(y() + (dy > 0 ? step : -step));
    } else {
        if (cp.waitTime > 0) {
            isWaiting = true;
            waitTicks = 0;
            waitTicksNeeded = qMax(1, (int)(cp.waitTime * 1000.0 / 32.0));
            waitPhase = WaitIdleBegin;
            switchToIdleBegin();
            return;
        }
        cpIndex++;
        if (cpIndex >= checkpoints.size()) {
            if (carrot)
                carrot->takeDamage(1);
            reachedEnd = true;
            hp = 0;
        }
    }
}
