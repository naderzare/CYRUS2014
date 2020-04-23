// press 

#include <rcsc/player/soccer_action.h>
#include <rcsc/geom/vector_2d.h>

#include <vector>

namespace rcsc {
class WorldModel;
}

class Bhv_Press
    : public rcsc::SoccerBehavior {

private:

    int traf;

public:

    Bhv_Press ( int temp ) { traf = temp;}
    bool execute        ( rcsc::PlayerAgent * agent         );
    bool dopress        ( rcsc::PlayerAgent * agent         );
    bool isOffensePress ( rcsc::PlayerAgent * agent,int num );


private:

};

