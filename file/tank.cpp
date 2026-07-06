#include "tank.h"

tank::tank()
{
    mobType = 3;
    speed = 30.0 * 32 / 1000.0;
    taunt = 1;
    setMovie(":/new/prefix1/images/tank_move.gif");
    setIdleMovie(":/new/prefix1/images/tank_idle.gif");
    setIdleBeginMovie(":/new/prefix1/images/tank_move_end.gif");
    setIdleEndMovie(":/new/prefix1/images/tank_move_begin.gif");
}
