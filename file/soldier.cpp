#include "soldier.h"

soldier::soldier()
{
    mobType = 1;
    hp = 50;
    speed = 120.0 * 32 / 1000.0;
    setMovie(":/new/prefix1/images/soldier_move.gif");
    setIdleMovie(":/new/prefix1/images/soldier_idle.gif");
}
