// -*-c++-*-

/*
 *Copyright:

 Copyright (C) Hidehisa AKIYAMA

 This code is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 3, or (at your option)
 any later version.

 This code is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this code; see the file COPYING.  If not, write to
 the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.

 *EndCopyright:
 */

/////////////////////////////////////////////////////////////////////

#ifndef SAMPLE_COACH_H
#define SAMPLE_COACH_H

#include <rcsc/coach/coach_agent.h>
#include <rcsc/types.h>

#include <vector>


namespace rcsc {
class PlayerType;
}


class SampleCoach
		: public rcsc::CoachAgent {
		private:
	typedef std::vector< const rcsc::PlayerType * > PlayerTypePtrCont;


	int M_opponent_player_types[11];

	rcsc::TeamGraphic M_team_graphic;

		public:
	static rcsc::Vector2D px[118][11];
	static rcsc::Vector2D bx[118];
	static int isPlayOn ;
	/////////////////////////////////////// possession /////////////////////////////////////

	//Cycle memory
	// if cycle is zero = 0
	// if goal = 1
	// if corner = 2
	// if foul && Free kick = 3
	// if kick in = 4
	// if tackle = 5
	// if shoot = 7
	// if save = 8
	// if goal kick = 9
	// if pass = 10
	// if dribble = 11
	static int cycleMemory ;
	static bool cycleMemoryChanged ;

	//who Kicked Last ??
	static bool weKickedLast;
	static bool oppKickedLast;
	static int kicker ;
	static int kicker2;
	static int passKicker ;
	static int passKicker2;
	static bool isKicker2;
	static bool kickable ;
	static int kickableNum ;
	static bool owner ;
	static bool oppKickable ;


	//Normal Possession

	static int ourPoss;
	static int theirPoss;
	static int playOnPoss;
	//Goals
	static int goalDiff ;
	static int ourGoals ;
	static int oppGoals ;

	//Corners
	static int ourCornerLeft ;
	static int ourCornerRight ;
	static int oppCornerLeft ;
	static int oppCornerRight ;

	//Dribble
	static int ourDribbles ;
	static int oppDribbles ;
	static int ourCorrectDribbles ;
	static int oppCorrectDribbles ;
	static int ourDribblesPercentage ;
	static int oppDribblesPercentage ;
	static bool weDribbled ;
	static bool oppDribbled ;

	//Passes
	static int ourPasses ;
	static int oppPasses ;
	static int ourCorrectPasses;
	static int oppCorrectPasses;
	static int ourPassesPercentage ;
	static int oppPassesPercentage ;
	static bool wePassed ;
	static bool oppPassed ;

	//Fouls
	static int ourFouls ;
	static int oppFouls ;

	//Yellow Cards
	static int ourYellowCards ;
	static int oppYellowCards ;

	//Outs
	static int ourKickIns ;
	static int oppKickIns ;

	//Tackles
	static int ourTackle ;
	static int oppTackle ;
	static int ourTackleCycle ;
	static int oppTackleCycle;

	//Free kicks
	static int ourFreeKick ;
	static int oppFreeKick ;
	static int ourFreeKickCycle ;
	static int oppFreeKickCycle ;
	static rcsc::Vector2D freeKickPoint ;

	//Shoots
	static int ourShoots ;
	static int oppShoots ;
	static bool weShot ;
	static bool oppShot;

	//Saves
	static int ourSaves ;
	static int oppSaves ;
	static int ourSaveCycle ;
	static int oppSaveCycle ;

	//Goal Kicks
	static int ourGoalKicks ;
	static int oppGoalKicks ;
	static int ourGoalKickCycle ;
	static int oppGoalKickCycle ;

	//Y_poss
	static int posYposs ;
	static int negYposs ;

	//Third positions
	static int ourThirdPoss ;
	static int middleThirdPoss ;
	static int oppThirdPoss ;

	//Relative Possession
	static int ourRelativePoss ;
	static int oppRelativePoss ;

	//Role Poss Our

	static int CB_ourPoss ;
	static int LB_ourPoss ;
	static int RB_ourPoss ;
	static int DM_ourPoss ;
	static int CM_ourPoss ;
	static int LM_ourPoss ;
	static int RM_ourPoss ;
	static int AM_ourPoss ;
	static int LW_ourPoss ;
	static int RW_ourPoss ;
	static int CF_ourPoss ;

	//Role Poss Opp

	static int CB_oppPoss ;
	static int LB_oppPoss ;
	static int RB_oppPoss ;
	static int DM_oppPoss ;
	static int CM_oppPoss ;
	static int LM_oppPoss ;
	static int RM_oppPoss ;
	static int AM_oppPoss ;
	static int LW_oppPoss ;
	static int RW_oppPoss ;
	static int CF_oppPoss ;

	//////////////////////////////////End of Possession ///////////////////////////////////
	SampleCoach();

	virtual
	~SampleCoach();


		protected:

	/*!
      You can override this method.
      But, CoachAgent::initImpl() must be called in this method.
	 */
	virtual
	bool initImpl( rcsc::CmdLineParser & cmd_parser );

	//! main decision making
	virtual
	void actionImpl();

	/*!
      this method is automatically called just after receiving server_param message.
	 */
	virtual
	void handleServerParam();

	/*!
      this method is automatically called just after receiving player_param message.
	 */
	virtual
	void handlePlayerParam();

	/*!
      this method is automatically called just after receiving player_type message.
	 */
	virtual
	void handlePlayerType();
	//////////////////////////// Possession /////////////////////////////

	void updateAnalyzer();
	void updatePasses();
	void updateDribbles();
	void updateGoals();
	void updateCorners();
	void updateFoulsAfreeKicks();
	void updateYellowCards();
	void updateKickIns();
	void updateTackles();
	void updateShoots();
	void updateSaves();
	void updateGoalKicks();
	void updateYposs();
	void updateThirdPoss ();
	void updateRoles();
	void updateRelativePoss();
	void updateTo541();
	void updateTo4123();
	void updateTo433();
	void updateMatchFacts();
	void findBallOwner();
	void weAreKickable();
	void oppIsKickable();
	void findLastKicker();

	//////////////////////// End of Possession /////////////////////////

		private:

	void doSubstitute();

	void doFirstSubstitute();
	void doSubstituteTiredPlayers();

	void substituteTo( const int unum,
			const int type );

	int getFastestType( PlayerTypePtrCont & candidates );

	void sayPlayerTypes();

	void sendTeamGraphic();

};

#endif
