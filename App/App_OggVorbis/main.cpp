#include "oggvorbisplayer.h"

int main(int argc, char *argv[])
{

    OggVorbisPlayer* player = OggVorbisPlayer::create(argv[1]);
    player->play();
    player->run();
    player->stop();

    return 0;
}
