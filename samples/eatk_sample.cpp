#include "EmbedATK/EmbedATK.h"

int main()
{
    EATK_INIT_LOG();

    EATK_INFO("Hallo welt {}", 5);

    for (int i = 0; i < 100; ++i) {
        EATK_INFO("Loop {}", i);
    } 

    EATK_SHUTDOWN_LOG();
    return 0;
}