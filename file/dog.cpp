#include "dog.h"

dog::dog()
{
    mobType = 2;
    speed = 240.0 * 32 / 1000.0;
    imageOffsetY = 12;
    setMovie(":/new/prefix1/images/dog_move.gif");
    setIdleMovie(":/new/prefix1/images/dog_idle.gif");
}
