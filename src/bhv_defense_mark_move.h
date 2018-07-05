







///////////////////////////////////////////////////////////M.Kh//////////////////////////////////////


#ifndef AGENT2D_DEFENSE_MARKMOVE
#define AGENT2D_DEFENSE_MARKMOVE



#include <rcsc/geom/vector_2d.h>
#include <rcsc/player/soccer_action.h>
#include <rcsc/player/player_object.h>
#include <rcsc/player/world_model.h>

class Defense_MarkMove
     {
private:
     rcsc::Vector2D home;

public:
	Defense_MarkMove( const rcsc::Vector2D & home_pos )

      { home = home_pos;}

    bool Stop_by_mark( rcsc::PlayerAgent * agent );
    bool danger_mark( rcsc::PlayerAgent * agent );
    bool mark_move( rcsc::PlayerAgent * agent );
    bool cross_mark ( rcsc::PlayerAgent * agent );
    bool ManToManMark ( rcsc::PlayerAgent * agent );
    bool PassCutMark ( rcsc::PlayerAgent * agent );
    const rcsc::AbstractPlayerObject * getMarkTarget( const rcsc::WorldModel & wm );
    rcsc::Vector2D getMarkPosition( const rcsc::WorldModel & wm,
                                    const rcsc::AbstractPlayerObject * target_player );

    void get_mark_pos( rcsc::PlayerAgent * agent , std::vector<const rcsc::AbstractPlayerObject * > *mark_table);
    static rcsc::Vector2D get_mark_pos_defmid( rcsc::PlayerAgent * agent );
    rcsc::Vector2D getManToManMarkMove ( rcsc::Vector2D tmp , rcsc::PlayerAgent * agent );
    double getDashPower( rcsc::PlayerAgent * agent , const rcsc::Vector2D & target_point );
    bool basicMove( rcsc::PlayerAgent * agent );

private:
  //  double getDashPower( const rcsc::PlayerAgent * agent,
                   //      const rcsc::Vector2D & target_point );
};

#endif
