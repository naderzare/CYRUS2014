// -*-c++-*-

/*!
 \file player_type.cpp
 \brief heterogenious player parametor Source File
 */

/*
 *Copyright:

 Copyright (C) Hidehisa AKIYAMA, Hiroki SHIMORA

 This code is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 3 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

 *EndCopyright:
 */

/////////////////////////////////////////////////////////////////////
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "player_type.h"

#include "player_param.h"
#include "server_param.h"
#include "stamina_model.h"

#include <rcsc/rcg/util.h>
#include <rcsc/player/world_model.h>

#include <sstream>
#include <iostream>
#include <algorithm>
#include <string>
#include <cstdio>
#include <cstring>
#include <cmath>

namespace rcsc {

/*-------------------------------------------------------------------*/
/*!

 */
PlayerType::PlayerType() :
		M_id(Hetero_Default) {
	setDefault();
	initAdditionalParams();
}

/*-------------------------------------------------------------------*/
/*!

 */
PlayerType::PlayerType(const char * server_msg, const double & version) :
		M_id(Hetero_Unknown) {
	setDefault();

	if (version >= 8.0) {
		parseV8(server_msg);
	} else {
		parseV7(server_msg);
	}

	initAdditionalParams();
}

/*-------------------------------------------------------------------*/
/*!

 */
PlayerType::PlayerType(const rcg::player_type_t & from) :
		M_id(Hetero_Unknown) {
	setDefault();

	M_id = rcg::nstohi(from.id);
	M_player_speed_max = rcg::nltohd(from.player_speed_max);
	M_stamina_inc_max = rcg::nltohd(from.stamina_inc_max);
	M_player_decay = rcg::nltohd(from.player_decay);
	M_inertia_moment = rcg::nltohd(from.inertia_moment);
	M_dash_power_rate = rcg::nltohd(from.dash_power_rate);
	M_player_size = rcg::nltohd(from.player_size);
	M_kickable_margin = rcg::nltohd(from.kickable_margin);
	M_kick_rand = rcg::nltohd(from.kick_rand);
	M_extra_stamina = rcg::nltohd(from.extra_stamina);
	M_effort_max = rcg::nltohd(from.effort_max);
	M_effort_min = rcg::nltohd(from.effort_min);

	if (from.kick_power_rate != 0)
		M_kick_power_rate = rcg::nltohd(from.kick_power_rate);
	if (from.foul_detect_probability != 0)
		M_foul_detect_probability = rcg::nltohd(from.foul_detect_probability);
	if (from.catchable_area_l_stretch != 0)
		M_catchable_area_l_stretch = rcg::nltohd(from.catchable_area_l_stretch);

	initAdditionalParams();
}

/*-------------------------------------------------------------------*/
/*!

 */
void PlayerType::convertTo(rcg::player_type_t & to) const {
	to.id = rcg::hitons(M_id);
	to.player_speed_max = rcg::hdtonl(M_player_speed_max);
	to.stamina_inc_max = rcg::hdtonl(M_stamina_inc_max);
	to.player_decay = rcg::hdtonl(M_player_decay);
	to.inertia_moment = rcg::hdtonl(M_inertia_moment);
	to.dash_power_rate = rcg::hdtonl(M_dash_power_rate);
	to.player_size = rcg::hdtonl(M_player_size);
	to.kickable_margin = rcg::hdtonl(M_kickable_margin);
	to.kick_rand = rcg::hdtonl(M_kick_rand);
	to.extra_stamina = rcg::hdtonl(M_extra_stamina);
	to.effort_max = rcg::hdtonl(M_effort_max);
	to.effort_min = rcg::hdtonl(M_effort_min);

	to.kick_power_rate = rcg::hdtonl(M_kick_power_rate);
	to.foul_detect_probability = rcg::hdtonl(M_foul_detect_probability);
	to.catchable_area_l_stretch = rcg::hdtonl(M_catchable_area_l_stretch);
}

/*-------------------------------------------------------------------*/
/*!

 */
std::string PlayerType::toStr() const {
	std::ostringstream os;

	os << "(player_type " << "(id " << M_id << ')' << "(player_speed_max "
			<< M_player_speed_max << ')' << "(stamina_inc_max "
			<< M_stamina_inc_max << ')' << "(player_decay " << M_player_decay
			<< ')' << "(inertia_moment " << M_inertia_moment << ')'
			<< "(dash_power_rate " << M_dash_power_rate << ')'
			<< "(player_size " << M_player_size << ')' << "(kickable_margin "
			<< M_kickable_margin << ')' << "(kick_rand " << M_kick_rand << ')'
			<< "(extra_stamina " << M_extra_stamina << ')' << "(effort_max "
			<< M_effort_max << ')' << "(effort_min " << M_effort_min << ')'
			<< "(kick_power_rate " << M_kick_power_rate << ')'
			<< "(foul_detect_probability " << M_foul_detect_probability << ')'
			<< "(catchable_area_l_stretch " << M_catchable_area_l_stretch << ')'
			<< ')';

	return os.str();
}

/*-------------------------------------------------------------------*/
/*!

 */
void PlayerType::setDefault() {
	const ServerParam & SP = ServerParam::instance();

	M_player_speed_max = SP.defaultPlayerSpeedMax();
	M_stamina_inc_max = SP.defaultStaminaIncMax();
	M_player_decay = SP.defaultPlayerDecay();
	M_inertia_moment = SP.defaultInertiaMoment();
	M_dash_power_rate = SP.defaultDashPowerRate();
	M_player_size = SP.defaultPlayerSize();
	M_kickable_margin = SP.defaultKickableMargin();
	M_kick_rand = SP.defaultKickRand();
	M_extra_stamina = SP.defaultExtraStamina();
	M_effort_max = SP.defaultEffortMax();
	M_effort_min = SP.defaultEffortMin();
	M_kick_power_rate = SP.kickPowerRate();
	M_foul_detect_probability = SP.foulDetectProbability();
	M_catchable_area_l_stretch = 1.0;
	updateCyrusTable();
}

/*-------------------------------------------------------------------*/
/*!

 */
void PlayerType::parseV8(const char * msg) {
	// read v8 protocol
	/*
	 "(player_type (id 0) (player_speed_max 1.2) (stamina_inc_max 45)
	 (player_decay 0.4) (inertia_moment 5)
	 (dash_power_rate 0.006) (player_size 0.3)
	 (kickable_margin 0.7) (kick_rand 0)
	 (extra_stamina 0) (effort_max 1) (effort_min 0.6))"
	 "(player_type (id 3) (player_speed_max 1.2) (stamina_inc_max 40.68)
	 (player_decay 0.5308) (inertia_moment 8.27)
	 (dash_power_rate 0.006432) (player_size 0.3)
	 (kickable_margin 0.7002) (kick_rand 0.0001)
	 (extra_stamina 98) (effort_max 0.804) (effort_min 0.404))";
	 */

	int n_read = 0;

	char name[32];
	int id = 0;
	if (std::sscanf(msg, " ( player_type ( %s %d ) %n ", name, &id, &n_read)
			!= 2 || n_read == 0 || id < 0) {
		std::cerr << "PlayerType id read error. " << msg << std::endl;
		return;
	}
	msg += n_read;

	M_id = id;

	int n_param = 0;
	while (*msg != '\0' && *msg != ')') {
		double val = 0.0;
		if (std::sscanf(msg, " ( %s %lf ) %n ", name, &val, &n_read) != 2
				|| n_read == 0) {
			std::cerr << "PlayerType parameter read error: " << msg
					<< std::endl;
			break;
		}
		msg += n_read;

		if (!std::strcmp(name, "player_speed_max")) {
			M_player_speed_max = val;
		} else if (!std::strcmp(name, "stamina_inc_max")) {
			M_stamina_inc_max = val;
		} else if (!std::strcmp(name, "player_decay")) {
			M_player_decay = val;
		} else if (!std::strcmp(name, "inertia_moment")) {
			M_inertia_moment = val;
		} else if (!std::strcmp(name, "dash_power_rate")) {
			M_dash_power_rate = val;
		} else if (!std::strcmp(name, "player_size")) {
			M_player_size = val;
		} else if (!std::strcmp(name, "kickable_margin")) {
			M_kickable_margin = val;
		} else if (!std::strcmp(name, "kick_rand")) {
			M_kick_rand = val;
		} else if (!std::strcmp(name, "extra_stamina")) {
			M_extra_stamina = val;
		} else if (!std::strcmp(name, "effort_max")) {
			M_effort_max = val;
		} else if (!std::strcmp(name, "effort_min")) {
			M_effort_min = val;
		} else if (!std::strcmp(name, "kick_power_rate")) {
			M_kick_power_rate = val;
		} else if (!std::strcmp(name, "foul_detect_probability")) {
			M_foul_detect_probability = val;
		} else if (!std::strcmp(name, "catchable_area_l_stretch")) {
			M_catchable_area_l_stretch = val;
		} else {
			std::cerr << "player_type: param name error " << msg << std::endl;
			break;
		}

		++n_param;
	}

	//std::cerr << "  read " << n_param << " params" << endl;
}

/*-------------------------------------------------------------------*/
/*!

 */
void PlayerType::parseV7(const char * msg) {
	// read v7 protocol
	// read param value only, no param name
	/*
	 from rcssserver/src/hetroplayer.C
	 std::ostream& toStr(std::ostream& o,
	 const PlayerTypeSensor_v7::data_t& data)
	 {
	 return o << "(player_type " << data.M_id << " "
	 << data.M_player_speed_max << " "
	 << data.M_stamina_inc_max << " "
	 << data.M_player_decay << " "
	 << data.M_inertia_moment << " "
	 << data.M_dash_power_rate << " "setOurPlayerType
	 << data.M_player_size << " "
	 << data.M_kickable_margin << " "
	 << data.M_kick_rand << " "
	 << data.M_extra_stamina << " "
	 << data.M_effort_max << " "
	 << data.M_effort_min
	 << ")";
	 }
	 */

	std::istringstream msg_strm(msg);
	std::string tmp;

	msg_strm
			>> tmp // skip "(player_type"
			>> M_id >> M_player_speed_max >> M_stamina_inc_max >> M_player_decay
			>> M_inertia_moment >> M_dash_power_rate >> M_player_size
			>> M_kickable_margin >> M_kick_rand >> M_extra_stamina
			>> M_effort_max >> M_effort_min;
}

/*-------------------------------------------------------------------*/
/*!

 */
void PlayerType::initAdditionalParams() {
	const ServerParam & SP = ServerParam::i();

	M_kickable_area = playerSize() + kickableMargin() + SP.ballSize();

	/////////////////////////////////////////////////////////////////////
	const double catch_stretch_length_x = (catchAreaLengthStretch() - 1.0)
			* SP.catchAreaLength();
	const double catch_length_min_x = SP.catchAreaLength()
			- catch_stretch_length_x;
	const double catch_length_max_x = SP.catchAreaLength()
			+ catch_stretch_length_x;

	const double catch_half_width2 = std::pow(SP.catchAreaWidth() / 2.0, 2);

	M_reliable_catchable_dist = std::sqrt(
			std::pow(catch_length_min_x, 2) + catch_half_width2);
	M_max_catchable_dist = std::sqrt(
			std::pow(catch_length_max_x, 2) + catch_half_width2);

	///////////////////////////////////////////////////////////////////
	double accel = SP.maxDashPower() * dashPowerRate() * effortMax();

	// see also soccer_math.h
	M_real_speed_max = accel / (1.0 - playerDecay()); // sum inf geom series
	if (M_real_speed_max > playerSpeedMax()) {
		M_real_speed_max = playerSpeedMax();
	}

	///////////////////////////////////////////////////////////////////
	M_player_speed_max2 = playerSpeedMax() * playerSpeedMax();
	M_real_speed_max2 = realSpeedMax() * realSpeedMax();

	/////////////////////////////////////////////////////////////////////
	double speed = 0.0;
	double dash_power = SP.maxDashPower();
	StaminaModel stamina_model;
	stamina_model.init(*this);

	double reach_dist = 0.0;

	M_cycles_to_reach_max_speed = -1;

	M_dash_distance_table.clear();
	M_dash_distance_table.reserve(50);

	for (int counter = 1; counter <= 50; ++counter) {
		if (speed + accel > playerSpeedMax()) {
			accel = playerSpeedMax() - speed;
			dash_power = std::min(SP.maxDashPower(),
					accel / (dashPowerRate() * stamina_model.effort()));
		}

		speed += accel;

		reach_dist += speed;

		M_dash_distance_table.push_back(reach_dist);

		if (M_cycles_to_reach_max_speed < 0 && speed >= realSpeedMax() - 0.01) {
			M_cycles_to_reach_max_speed = counter;
		}

		speed *= playerDecay();

		stamina_model.simulateDash(*this, dash_power);

		if (stamina_model.stamina() <= 0.0) {
			break;
		}
	}
	updateCyrusTable();
}

bool log = false;
//double PlayerType::distanceTable[19][12][50] = { 0 }; //angleDif from body// first speed*10-->0 ta 11 // cycle
//int PlayerType::playerTypeForCyrus = -1;
using namespace std;
void PlayerType::updateCyrusTable() {

	if (gettype() == id())
		return;
	for (int p = 0; p < 23; p++)
		for (int a = 0; a < 18; a++)
			for (int s = 0; s < 12; s++)
				for (int c = 0; c < 50; c++)
					distanceTable[a][s][c] = 0;
	this->playerTypeForCyrus = id();

	const ServerParam & sp = ServerParam::i();
	Vector2D first_pos = Vector2D(0, 0);
	StaminaModel stamina_model;
	stamina_model.init(*this);

	for (double a = 0; a < 19; a++) {
		double dash_angle = a * 10.0;
		Vector2D accel_unit = Vector2D::polar2vector(1.0, dash_angle);
		double accel_d = ServerParam::i().maxDashPower()
				* dashRate(stamina_model.effort()) * sp.dashDirRate(dash_angle);

		double max_speed = accel_d / (1.0 - playerDecay());
		for (double s = 0; s < 12; s++) {
			Vector2D vel = Vector2D::polar2vector(s / (10.0), dash_angle);
			Vector2D pos = first_pos;
			for (int i = 1; i < 50; i++) {
				Vector2D accel = accel_unit * accel_d;
				if ((accel + vel).r() > max_speed)
					vel = Vector2D::polar2vector(max_speed, dash_angle);
				else
					vel += accel;
				pos += vel;
				vel *= playerDecay();
				double dist = pos.dist(first_pos);
				distanceTable[(int) a][(int) s][i] = dist;
			}
		}
	}
	if (log)
		cout << "**********************************************" << std::endl;
	if (log)
		cout << "id : " << id() << endl;
	for (double a = 0; a < 19; a++) {
		double accel_d = ServerParam::i().maxDashPower()
				* dashRate(stamina_model.effort()) * sp.dashDirRate(a * 10.0);

		if (log)
			cout << " angle " << a * 10.0 << " accel " << accel_d << std::endl;

		for (int s = 0; s < 12; s++) {
			if (log)
				cout << "speed : " << s << endl;
			for (int i = 0; i < 10; i++) {
				if (log)
					cout << distanceTable[(int) a][s][i] << " ";
			}
			if (log)
				cout << std::endl;
		}
		if (log)
			cout << "----------------------------------------" << std::endl;
	}
	if (log)
		cout << "**********************************************" << std::endl;
}

int PlayerType::cyruscycle2target(const WorldModel & wm,
		const AbstractPlayerObject * p, Vector2D target, bool istm, bool pass,
		double alfathr, int kick_count, bool cross) const {

//	cout << wm.time().cycle() << "p" << (istm ? p->unum() : -(p->unum())) << "t"
//			<< playerTypeForCyrus << "id" << id() << endl;
//	updateCyrusTable();

	kick_count=max(kick_count,0);
	bool have_turn = false;
	int min = 1000;
	if (p->unum() < 1)
		return 1000;
	double thr = (
			(p->goalie() && target.x > 40 && target.absY() < 15 && !istm) ?
					catchAreaLengthStretch() : kickableArea()) * alfathr;
	if (istm)
		thr = kickableArea() * (2 - alfathr);
	bool havback = (!istm && p->pos().dist(target) < 5)
			|| p->pos().dist(target) < 1;
	double pendist = 0;
	double bondist = 0;

	if (pass) {
		if (istm)
			pendist = getpenalty(p, target, havback, kick_count) * alfathr;
		else
			bondist = getbonus(p, target, havback, kick_count) * alfathr;
	}

	if (pass) {
		if (istm) {
			int type1 = cyruscycleturnturndash(p, target, thr, 10, istm,
					have_turn, havback, bondist, pendist, alfathr);
			if (type1 < min)
				min = type1;

		} else {

			if (log)
				cout << "-      opp" << p->unum() << " to " << target
						<< " posco : " << p->posCount() << " bonus : "
						<< bondist << endl;
			int type1 = cyruscycleturnturndash(p, target, thr, 18, istm,
					have_turn, havback, bondist, pendist, alfathr);
			if (type1 < min)
				min = type1;

			int type3 = cyruscycleturnturndash(p, target, thr, 5, istm,
					have_turn, havback, bondist, pendist, alfathr);
			if (type3 < min)
				min = type3;

			/*if (p->pos().dist(target) < 5) {
			 int type2 = cyruscycleturnorivdash(p, target, thr, 90, istm,
			 have_turn, havback, bondist, pendist);
			 if (type2 < min)
			 min = type2;
			 }*/
		}
	}else if(cross){
		if(istm){
			int type1 = cyruscycleturnturndash(p, target, thr, 10, istm,
					have_turn, havback, bondist, pendist/7, alfathr);
			if (type1 < min)
				min = type1;

		}else{
			int type1 = cyruscycleturnturndash(p, target, thr, 18, istm,
					have_turn, havback, bondist/7, pendist, alfathr);
			if (type1 < min)
				min = type1;

			int type3 = cyruscycleturnturndash(p, target, thr, 5, istm,
					have_turn, havback, bondist/7, pendist, alfathr);
			if (type3 < min)
				min = type3;

		}
	}
	else{
		int type1 = cyruscycleturnturndash(p, target, thr, 18, istm,
				have_turn, havback, 0, pendist, alfathr);
		if (type1 < min)
			min = type1;

		int type3 = cyruscycleturnturndash(p, target, thr, 5, istm,
				have_turn, havback, 0, pendist, alfathr);
		if (type3 < min)
			min = type3;

	}
	return min;
}

double PlayerType::getbonus(const AbstractPlayerObject * p, Vector2D target,
		bool havback, int kick_count) const {
	double bondist = 0;
	Vector2D vel = p->vel();
	Vector2D pos = p->pos();

	double vel_deg = vel.th().degree();
	double body_deg = p->body().degree();
	double target_deg = (target - pos).th().degree();

	if (vel.r() < 0.1)
		vel_deg = body_deg;

	double player_deg = (vel_deg + body_deg) / 2.0;
	double player_dif_target = abs(target_deg - player_deg);
	if (player_dif_target > 180)
		player_dif_target = 360 - player_dif_target;

	double body_dif_target = abs(target_deg - body_deg);
	if (havback && body_dif_target > 90)
		player_dif_target -= 90;

	if (player_dif_target < 5) {

		if (vel.r() > 0.3)
			bondist = estimate_virtual_dash_distance(p, 30 + kick_count);
		else if (vel.r() > 0.2)
			bondist = estimate_virtual_dash_distance(p, 26 + kick_count);
		else if (vel.r() > 0.1)
			bondist = estimate_virtual_dash_distance(p, 22 + kick_count);
		else
			bondist = estimate_virtual_dash_distance(p, 20 + kick_count);

	} else if (player_dif_target < 15) {

		if (vel.r() > 0.3)
			bondist = estimate_virtual_dash_distance(p, 23 + kick_count);
		else if (vel.r() > 0.2)
			bondist = estimate_virtual_dash_distance(p, 20 + kick_count);
		else if (vel.r() > 0.1)
			bondist = estimate_virtual_dash_distance(p, 18 + kick_count);
		else
			bondist = estimate_virtual_dash_distance(p, 14 + kick_count);

	} else if (player_dif_target < 35) {

		if (vel.r() > 0.3)
			bondist = estimate_virtual_dash_distance(p, 19 + kick_count);
		else if (vel.r() > 0.2)
			bondist = estimate_virtual_dash_distance(p, 16 + kick_count);
		else if (vel.r() > 0.1)
			bondist = estimate_virtual_dash_distance(p, 16 + kick_count);
		else
			bondist = estimate_virtual_dash_distance(p, 17 + kick_count);

	} else {

		if (vel.r() > 0.3)
			bondist = estimate_virtual_dash_distance(p, 12 + kick_count);
		else if (vel.r() > 0.2)
			bondist = estimate_virtual_dash_distance(p, 14 + kick_count);
		else if (vel.r() > 0.1)
			bondist = estimate_virtual_dash_distance(p, 15 + kick_count);
		else
			bondist = estimate_virtual_dash_distance(p, 15 + kick_count);

	}

	return bondist;
}
double PlayerType::getpenalty(const AbstractPlayerObject * p, Vector2D target,
		bool havback, int kick_count) const {

	double pendist = 0;
	Vector2D vel = p->vel();
	Vector2D pos = p->pos();

	double vel_deg = vel.th().degree();
	double body_deg = p->body().degree();
	double target_deg = (target - pos).th().degree();

	if (vel.r() < 0.1)
		vel_deg = body_deg;

	double player_deg = (vel_deg + body_deg) / 2.0;
	double player_dif_target = abs(target_deg - player_deg);
	if (player_dif_target > 180)
		player_dif_target = 360 - player_dif_target;

	double body_dif_target = abs(target_deg - body_deg);
	if (havback && body_dif_target > 90)
		player_dif_target -= 90;

	if (player_dif_target < 5) {

		if (vel.r() > 0.3)
			pendist = estimate_virtual_dash_distance(p, 8, false);
		else if (vel.r() > 0.2)
			pendist = estimate_virtual_dash_distance(p, 12, false);
		else if (vel.r() > 0.1)
			pendist = estimate_virtual_dash_distance(p, 15, false);
		else
			pendist = estimate_virtual_dash_distance(p, 18, false);

	} else if (player_dif_target < 15) {

		if (vel.r() > 0.3)
			pendist = estimate_virtual_dash_distance(p, 13, false);
		else if (vel.r() > 0.2)
			pendist = estimate_virtual_dash_distance(p, 15, false);
		else if (vel.r() > 0.1)
			pendist = estimate_virtual_dash_distance(p, 18, false);
		else
			pendist = estimate_virtual_dash_distance(p, 20, false);

	} else if (player_dif_target < 35) {

		if (vel.r() > 0.3)
			pendist = estimate_virtual_dash_distance(p, 16, false);
		else if (vel.r() > 0.2)
			pendist = estimate_virtual_dash_distance(p, 18, false);
		else if (vel.r() > 0.1)
			pendist = estimate_virtual_dash_distance(p, 20, false);
		else
			pendist = estimate_virtual_dash_distance(p, 21, false);

	} else {

		if (vel.r() > 0.3)
			pendist = estimate_virtual_dash_distance(p, 27, false);
		else if (vel.r() > 0.2)
			pendist = estimate_virtual_dash_distance(p, 24, false);
		else if (vel.r() > 0.1)
			pendist = estimate_virtual_dash_distance(p, 22, false);
		else
			pendist = estimate_virtual_dash_distance(p, 21, false);

	}

	return pendist;
}

double PlayerType::estimate_virtual_dash_distance(
		const rcsc::AbstractPlayerObject * player, double magic,
		bool teammate) const {
	int pos_count = std::min(8, // Magic Number
			player->posCount());

	const double max_speed = realSpeedMax() * 0.9; // Magic Number

	double d = 0.0;
	if (teammate) {
		pos_count++;
	}
	for (int i = 1; i <= pos_count; ++i) // start_value==1 to set the initial_value<1
			{
		d += max_speed * std::exp(-(i * i) / magic); // Magic Number
	}

	return d;
}

using namespace std;
int PlayerType::cyruscycleturnturndash(const AbstractPlayerObject * p,
		Vector2D target, double distThr, double anglethr, bool istm,
		bool have_turn, bool have_back, double bonus_dist, double penalty_dist,
		double alfathr) const { //turn harekat mostaghim ba zavie badan

	if (log)
		cout << "-       ----------------->>>>>>>>>>>" << endl;
	if (p->unum() < 1)
		return 1000;
	Vector2D vel = p->vel();
	Vector2D pos = p->pos();
	Vector2D velTarget = vel;
	double body = p->body().degree();

	double dist = pos.dist(target) - bonus_dist + penalty_dist
			- (istm ? 0 : 0.15);
	if (log)
		cout << "-      pos : " << pos << " vel : " << vel << " body : " << body
				<< " dist : " << dist << endl;
	if (dist < distThr)
		return 0;

	int n_turn = cycleTurn(p, target, anglethr, &vel, &pos, &velTarget, &body,
			have_back, false);

	dist = pos.dist(target) - bonus_dist + penalty_dist;

	if (n_turn > 0) {
		have_turn = true;
	}
	bool tackle = false;
	if (!have_back && !istm)
		tackle = true;

	double tvel = velTarget.r();
	bool vel_manfi = false;
	if ((velTarget + (target - pos)).r() < (target - pos).r()) {
		if (!istm) {
			tvel = 0;
			velTarget = Vector2D(0, 0);
		} else {
			tvel = 0;
			velTarget = Vector2D(0, 0);
			vel_manfi = true;
		}
	}
	if (log)
		cout << "-      after turn : " << n_turn << " pos : " << pos
				<< " vel : " << vel << " velt : " << velTarget << " body : "
				<< body << " haveback :" << (have_back ? "t" : "f")
				<< " havetackle :" << (tackle ? "t" : "f") << endl;

	Line2D dashLine = Line2D(pos, body);
	double new_dist = dashLine.projection(target).dist(pos) - bonus_dist
			+ penalty_dist;
	dist = new_dist;
	double dist_after_dash = dashLine.dist(target);
	if (log)
		cout << "-      dist after dash : " << dist_after_dash;
	int cycle_for_orivdash = 0;

	if (dist < distThr && istm) {
		return n_turn + (n_turn > 1 ? 1 : 0)/*+(vel_manfi?1:0)*/;
	}
	if (dist < distThr && !istm) {
		return n_turn /*+(vel_manfi?1:0)*/;
	}

	if (dist_after_dash < 0.5) {
		dist_after_dash = 0;
		if (tackle) {
			dist -= (ServerParam::i().tackleDist());
			if (log)
				cout << " near tackle dist : " << dist << endl;
		} else {
			dist -= distThr;
			if (log)
				cout << " near no tackle dist : " << dist << endl;
		}

	} else if (dist_after_dash < distThr) {
		dist += dist_after_dash / 2;
		dist_after_dash = 0;
		if (tackle) {
			dist -= (ServerParam::i().tackleDist());
			if (log)
				cout << " thr tackle dist : " << dist << endl;
		} else {

			dist -= distThr;
			if (log)
				cout << " thr no tackle dist : " << dist << endl;
		}
	} else {
		if (tackle) {
			dist_after_dash -= (ServerParam::i().tackleDist());
			if (log)
				cout << " far tackle dist af d : " << dist_after_dash << endl;
		} else {
			dist_after_dash -= distThr;
			if (log)
				cout << " far no tackle dist af d : " << dist_after_dash
						<< endl;
		}
	}

	if (dist < 0) {
		return n_turn + (tackle ? 1 : 0);
	}
	if (dist_after_dash != 0) {
		cycle_for_orivdash = ceil(dist_after_dash) + 1; //1 for eror //1 for turn
		if (istm)
			cycle_for_orivdash++;
	}
	if (cycle_for_orivdash > 3 && !istm)
		cycle_for_orivdash = 3;

	if (log)
		cout << "-      cycle for oriv : " << cycle_for_orivdash << endl;
	bool have_view = false;
	if ((n_turn > 1 || tvel < 0.07) && p->posCount() < 5 && !istm)
		have_view = true;
	if (/*(have_turn || tvel < 0.1) && */istm)
		have_view = true;

	if (alfathr > 1.1 && !istm) {
		have_view = false;
	}
	int velindex = tvel * 10;
	if (log)
		cout << " veltarget : " << tvel << endl;

	int angleindex = 0;
	for (int i = 0; i <= 10; i++) {
		if (dist < distanceTable[angleindex][velindex][i]) {

			if (log)
				cout << "-      " << " i : " << i << " turn : " << n_turn
						<< " oriv: " << cycle_for_orivdash << " hv "
						<< (have_view ? 1 : 0) << " ht " << (tackle ? 1 : 0)
						<< " oriv turn "
						<< ((istm && cycle_for_orivdash > 0) ? 1 : 0)
						<< " oriv turn "
						<< ((!istm && cycle_for_orivdash > 1) ? 1 : 0)
						<< " vel manfi " << ((vel_manfi && istm) ? 1 : 0)
						<< " = "
						<< i + n_turn + cycle_for_orivdash + (have_view ? 1 : 0)
								+ (tackle ? 1 : 0)
								+ ((istm && cycle_for_orivdash > 0) ? 1 : 0)
								+ ((!istm && cycle_for_orivdash > 1) ? 1 : 0)
								+ ((vel_manfi && istm) ? 1 : 0) << endl;
			return i + n_turn + cycle_for_orivdash + (have_view ? 1 : 0)
					+ (tackle ? 1 : 0)
					+ ((istm && cycle_for_orivdash > 0) ? 1 : 0)
					+ ((!istm && cycle_for_orivdash > 1) ? 1 : 0)
					+ ((vel_manfi && istm) ? 1 : 0);
		}
	}
	dist -= distanceTable[angleindex][velindex][10];

	StaminaModel stamina_model;
	stamina_model.init(*this);
	double accel_d = ServerParam::i().maxDashPower()
			* dashRate(stamina_model.effort())
			* ServerParam::i().dashDirRate(0);
	double max_speed = accel_d / (1.0 - playerDecay());

	if (log)
		cout << "-      maxspeed : " << max_speed << endl;
	int cycle_with_max = ceil(dist / max_speed);

	if (log)
		cout << "-      " << " w max : " << cycle_with_max << " i : " << 10
				<< " turn : " << n_turn << " oriv: " << cycle_for_orivdash
				<< " hv " << (have_view ? 1 : 0) << " ht " << (tackle ? 1 : 0)
				<< " oriv turn " << ((istm && cycle_for_orivdash > 0) ? 1 : 0)
				<< " oriv turn " << ((!istm && cycle_for_orivdash > 1) ? 1 : 0)
				<< " vel manfi " << ((vel_manfi && istm) ? 1 : 0) << " = "
				<< cycle_with_max + 10 + n_turn + cycle_for_orivdash
						+ (have_view ? 1 : 0) + (tackle ? 1 : 0)
						+ ((istm && cycle_for_orivdash > 0) ? 1 : 0)
						+ ((!istm && cycle_for_orivdash > 1) ? 1 : 0)
						+ ((vel_manfi && istm) ? 1 : 0) << endl;
	return cycle_with_max + 10 + n_turn + cycle_for_orivdash
			+ (have_view ? 1 : 0) + (tackle ? 1 : 0)
			+ ((istm && cycle_for_orivdash > 0) ? 1 : 0)
			+ ((!istm && cycle_for_orivdash > 1) ? 1 : 0)
			+ ((vel_manfi && istm) ? 1 : 0);
}

int PlayerType::cyruscycleturnorivdash(const AbstractPlayerObject * p,
		Vector2D target, double distThr, double anglethr, bool istm,
		bool have_turn, bool have_back, double bonus_dist, double penalty_dist,
		double alfathr) const {
	if (p->unum() < 1)
		return 1000;

	if (log)
		cout << "/       //////////////////>>>>>>>>>>>" << endl;
	Vector2D vel = p->vel();
	Vector2D pos = p->pos();
	Vector2D velTarget = vel;
	double body = p->body().degree();

	double dist = pos.dist(target) - bonus_dist + penalty_dist
			- (istm ? 0 : 0.15);

	if (log)
		cout << "/      pos : " << pos << " vel : " << vel << " body : " << body
				<< " dist : " << dist << endl;

	if (dist < distThr)
		return 0;

	int n_turn = cycleTurn(p, target, anglethr, &vel, &pos, &velTarget, &body,
			have_back, true);
	if (n_turn > 0)
		have_turn = true;

	dist = pos.dist(target) - bonus_dist + penalty_dist;

	if (dist < distThr)
		return 0;

	double body_dif_target = abs((target - pos).th().degree() - body);
	if (body_dif_target > 180)
		body_dif_target = 360 - body_dif_target;

	bool tackle = false;
	if (!have_back && !istm && body_dif_target < 50)
		tackle = true;

	double tvel = velTarget.r();
	if ((velTarget + (target - pos)).r() < (target - pos).r())
		tvel = 0;

	if (log)
		cout << "/      after turn : " << n_turn << " pos : " << pos
				<< " vel : " << vel << " velt : " << velTarget << " body : "
				<< body << " haveback :" << (have_back ? "t" : "f")
				<< " havetackle :" << (tackle ? "t" : "f") << endl;

	if (tackle) {
		if (body_dif_target < 15)
			dist -= (ServerParam::i().tackleDist() - 0.2);
		else if (body_dif_target < 30)
			dist -= (ServerParam::i().tackleDist() - 0.5);
		else
			dist -= (ServerParam::i().tackleDist() - 1.0);
		if (log)
			cout << "/      after tackle dist : " << dist << endl;
	} else {
		dist -= distThr;
		if (log)
			cout << "/      after kick dist : " << dist << endl;
	}

	if (dist < 0) {
		return n_turn + (tackle ? 1 : 0);
	}

	bool have_view = false;
	if ((n_turn > 1 || tvel == 0) && p->posCount() == 0 && !istm)
		have_view = true;
	if ((have_turn || tvel < 0.1) && istm)
		have_view = false;

	int velindex = tvel * 10;

	double targetangle = abs(body - (target - pos).th().degree());
	if (targetangle > 180)
		targetangle = 360 - targetangle;

	int angleindex = targetangle / 10;
	for (int i = 0; i <= 10; i++) {
		if (dist < distanceTable[angleindex][velindex][i]) {
			if (log)
				cout << "/      i : " << i << " nturn : " << n_turn << " hv: "
						<< (have_view ? 1 : 0) << " ht : " << (tackle ? 1 : 0)
						<< " with angle : " << targetangle << endl;
			return i + n_turn + (have_view ? 1 : 0) + (tackle ? 1 : 0);
		}
	}
	dist -= distanceTable[angleindex][velindex][10];

	StaminaModel stamina_model;
	stamina_model.init(*this);
	double accel_d = ServerParam::i().maxDashPower()
			* dashRate(stamina_model.effort())
			* ServerParam::i().dashDirRate(targetangle);
	double max_speed = accel_d / (1.0 - playerDecay());

	int cycle_with_max = ceil(dist / max_speed);
	if (log)
		cout << "/      i : " << 10 << " cycle max :" << cycle_with_max
				<< " nturn : " << n_turn << " hv: " << (have_view ? 1 : 0)
				<< " ht : " << (tackle ? 1 : 0) << " with angle : "
				<< targetangle << endl;
	return cycle_with_max + 10 + n_turn + (have_view ? 1 : 0) + (tackle ? 1 : 0);
}

int PlayerType::cycleTurn(const AbstractPlayerObject * p, Vector2D target,
		double angleThr, Vector2D * newSpeed, Vector2D * newPos,
		Vector2D * velTarget, double * newBody, bool & back_dash,
		bool orivdash) const {
	const ServerParam & SP = ServerParam::i();
	double speed = p->vel().r();
	double bodyDeg = p->body().degree();
	double targetDeg = (target - (p->pos() + p->vel())).th().degree();

	double angle_diff = targetDeg - bodyDeg; //[-180,180]
	if (angle_diff > 180)
		angle_diff = angle_diff - 360;
	else if (angle_diff < -180)
		angle_diff = 360 + angle_diff;

	double decay = playerDecay(); // 0.4

	int n_turn = 0;
	if (back_dash && abs(angle_diff) > 90) {
		if (angle_diff > 0)
			angle_diff -= 180;
		else
			angle_diff += 180;
	} else {
		back_dash = false;
	}

	double turnMoment = 0;
	(*newSpeed) = p->vel();
	AngleDeg player_body = p->body();
	if (back_dash) {
		if (player_body.degree() > 0)
			player_body -= 180;
		else
			player_body += 180;
	}
	(*newPos) = p->pos();

	while (abs(angle_diff) > angleThr) {
		turnMoment = std::min(
				p->playerTypePtr()->effectiveTurn(SP.maxMoment(), speed),
				abs(angle_diff));
		//[0,180]
		if (angle_diff < 0) {
			player_body -= turnMoment;
			angle_diff += turnMoment;
		} else {
			player_body += turnMoment;
			angle_diff -= turnMoment;
		}
		(*newPos) += (*newSpeed);
		speed *= decay;
		(*newSpeed) *= decay;
		++n_turn;

	}
	if (n_turn == 0) {
		Vector2D vell = (*newSpeed);
		if (orivdash) {
			if (log)
				cout << vell.th() << " " << vell.th().degree() << " "
						<< targetDeg;
			double difdeg = abs(vell.th().degree() - targetDeg);
			if (difdeg > 180)
				difdeg = 360 - difdeg;
			if (difdeg > 90)
				difdeg = 180 - difdeg;
			if (log)
				cout << " " << difdeg << " " << cos(difdeg * 3.14 / 180.0)
						<< endl;
			(*velTarget) = vell * cos(difdeg * 3.14 / 180.0);
			(*newSpeed) = vell - (*velTarget);
			(*newPos) += (*newSpeed);
			(*newPos) += ((*newSpeed) * playerDecay());
			(*newPos) += ((*newSpeed) * playerDecay() * playerDecay());

		} else {
			if (log)
				cout << vell.th() << " " << vell.th().degree() << " "
						<< bodyDeg;
			double difdeg = abs(vell.th().degree() - bodyDeg);
			if (difdeg > 180)
				difdeg = 360 - difdeg;
			if (difdeg > 90)
				difdeg = 180 - difdeg;
			if (log)
				cout << " " << difdeg << " " << cos(difdeg * 3.14 / 180.0)
						<< endl;
			(*velTarget) = vell * cos(difdeg * 3.14 / 180.0);
			(*newSpeed) = vell - (*velTarget);
			(*newPos) += (*newSpeed);
			(*newPos) += ((*newSpeed) * playerDecay());
			(*newPos) += ((*newSpeed) * playerDecay() * playerDecay());
		}
		return 0;
	}
	if (orivdash) {
		Vector2D vell = (*newSpeed);

		if (log)
			cout << vell.th() << " " << targetDeg;
		double difdeg = abs(vell.th().degree() - targetDeg);
		if (difdeg > 180)
			difdeg = 360 - difdeg;
		if (difdeg > 90)
			difdeg = 180 - difdeg;
		if (log)
			cout << " " << difdeg << " " << cos(difdeg * 3.14 / 180.0) << endl;
		(*velTarget) = vell * cos(difdeg * 3.14 / 180.0);
		(*newSpeed) = vell - (*velTarget);
		(*newPos) += (*newSpeed);
		(*newPos) += ((*newSpeed) * playerDecay());
		(*newPos) += ((*newSpeed) * playerDecay() * playerDecay());

	} else {
		Vector2D vell = (*newSpeed);

		if (log)
			cout << vell.th() << " " << player_body;
		double difdeg = abs(vell.th().degree() - player_body.degree());
		if (difdeg > 180)
			difdeg = 360 - difdeg;
		if (difdeg > 90)
			difdeg = 180 - difdeg;
		if (log)
			cout << " " << difdeg << " " << cos(difdeg * 3.14 / 180.0) << endl;
		(*velTarget) = vell * cos(difdeg * 3.14 / 180.0);
		(*newSpeed) = vell - (*velTarget);
		(*newPos) += (*newSpeed);
		(*newPos) += ((*newSpeed) * playerDecay());
		(*newPos) += ((*newSpeed) * playerDecay() * playerDecay());
	}
	(*newBody) = player_body.degree();
	return n_turn;

}
/*-------------------------------------------------------------------*/
/*!

 */
double PlayerType::reliableCatchableDist() const {
	if (ServerParam::i().catchProbability() < 1.0) {
		return 0.0;
	} else {
		return M_reliable_catchable_dist;
	}
}

/*-------------------------------------------------------------------*/
/*!

 */
double PlayerType::reliableCatchableDist(const double prob) const {
	if (prob > 1.0) {
		std::cerr << "internal error at PlayerType::reliableCatchableDist(): "
				<< "probability " << prob << " too big" << std::endl;

		return reliableCatchableDist();
	} else if (prob < 0.0) {
		std::cerr << "internal error at PlayerType::reliableCatchableDist(): "
				<< "probability " << prob << " too small" << std::endl;

		return maxCatchableDist();
	}

	const ServerParam & SP = ServerParam::i();

	const double target_prob = prob / SP.catchProbability();
	if (target_prob > 1.0) {
		return 0.0;
	}

	const double catch_stretch_length_x = (catchAreaLengthStretch() - 1.0)
			* SP.catchAreaLength();
	const double catch_length_min_x = SP.catchAreaLength()
			- catch_stretch_length_x;

	const double dist_x = catch_length_min_x
			+ (catch_stretch_length_x * 2.0 * (1.0 - target_prob));

	const double diagonal_length = std::sqrt(
			std::pow(dist_x, 2) + std::pow(SP.catchAreaWidth() / 2.0, 2));
	return diagonal_length;
}
/*-------------------------------------------------------------------*/
/*!

 */
double PlayerType::getCatchProbability(const double dist) const {
	const ServerParam & SP = ServerParam::i();

	if (dist < M_reliable_catchable_dist) {
		if (dist < 0.0) {
			std::cerr
					<< "internal error at PlayerType::getCatchProbability(dist): negative distance "
					<< dist << std::endl;
		}

		return SP.catchProbability();
	} else if (dist > M_max_catchable_dist) {
		return 0.0;
	}

	const double catch_stretch_length_x = (catchAreaLengthStretch() - 1.0)
			* SP.catchAreaLength();
	const double catch_length_min_x = SP.catchAreaLength()
			- catch_stretch_length_x;

	const double dist_x = std::sqrt(
			std::pow(dist, 2) - std::pow(SP.catchAreaWidth() / 2.0, 2));

	const double fail_prob = (dist_x - catch_length_min_x)
			/ (catch_stretch_length_x * 2.0);

	return (1.0 - fail_prob) * SP.catchProbability();
}

/*-------------------------------------------------------------------*/
/*!

 */
int PlayerType::getMaxDashCyclesSavingRecovery(const double & dash_power,
		const double & current_stamina, const double & current_recovery) const {
	double available_stamina = current_stamina
			- ServerParam::i().recoverDecThrValue() - 1.0;
	double used_stamina = (dash_power > 0.0 ? dash_power : dash_power * -2.0);

	available_stamina -= used_stamina; // buffer for last one dash
	if (available_stamina < 0.0) {
		return 0;
	}

	double one_cycle_consumpation = used_stamina
			- (staminaIncMax() * current_recovery);
	return static_cast<int>(std::floor(
			available_stamina / one_cycle_consumpation));
}

/*-------------------------------------------------------------------*/
/*!

 */
#if 0
int
PlayerType::maxDashCyclesWith( const double & stamina ) const
{
	if ( stamina <= 0.0 )
	{
		return 0;
	}

	std::vector< double >::const_iterator
	it = std::lower_bound( M_stamina_table.begin(),
			M_stamina_table.end(),
			stamina );

	if ( it != M_dash_distance_table.end() )
	{
		return static_cast< int >( it - M_stamina_table.begin() );
	}

	if ( stamina < M_stamina_table.front() )
	{
		return 0;
	}

	return M_stamina_table.size();
}
#endif

/*-------------------------------------------------------------------*/
/*!

 */
#if 0
double
PlayerType::consumedStaminaAfterNrDash( const int n_dash ) const
{
	if ( 0 <= n_dash && n_dash < (int)M_stamina_table.size() )
	{
		return M_stamina_table[n_dash];
	}

	return M_stamina_table.back();
}
#endif

/*-------------------------------------------------------------------*/
/*!

 */
int PlayerType::cyclesToReachMaxSpeed(const double & dash_power) const {
	double accel = std::fabs(dash_power) * dashPowerRate() * effortMax();
	double speed_max = accel / (1.0 - playerDecay());
	if (speed_max > playerSpeedMax()) {
		speed_max = playerSpeedMax();
	}

	double decn = 1.0 - ((speed_max - 0.01) * (1.0 - playerDecay()) / accel);
	return static_cast<int>(std::ceil(std::log(decn) / std::log(playerDecay())));
}

/*-------------------------------------------------------------------*/
/*!

 */
int PlayerType::cyclesToReachDistance(const double & dash_dist) const {
	if (dash_dist <= 0.001) {
		return 0;
	}

	std::vector<double>::const_iterator it = std::lower_bound(
			M_dash_distance_table.begin(), M_dash_distance_table.end(),
			dash_dist - 0.001);

	if (it != M_dash_distance_table.end()) {
		return (static_cast<int>(std::distance(M_dash_distance_table.begin(),
				it)) + 1); // is it necessary?
	}

	double rest_dist = dash_dist - M_dash_distance_table.back();
	int cycle = M_dash_distance_table.size();

	cycle += static_cast<int>(std::ceil(rest_dist / realSpeedMax()));

	return cycle;
}

/*-------------------------------------------------------------------*/
/*!

 */
double PlayerType::getDashPowerToKeepMaxSpeed(const double & effort) const {
	// required accel in 1 step to keep max speed
	double required_power = playerSpeedMax() * (1.0 - playerDecay());
	// required dash power to keep max speed
	required_power /= (effort * dashPowerRate());

	return std::min(required_power, ServerParam::i().maxDashPower());
}

/*-------------------------------------------------------------------*/
/*!

 */
double PlayerType::kickRate(const double & ball_dist,
		const double & dir_diff) const {
	return (kickPowerRate() //ServerParam::i().kickPowerRate()
			* (1.0 - 0.25 * std::fabs(dir_diff) / 180.0
					- (0.25
							* (ball_dist - ServerParam::i().ballSize()
									- playerSize()) / kickableMargin())));
}

/*-------------------------------------------------------------------*/
/*!

 */
double PlayerType::dashRate(const double & effort,
		const double & rel_dir) const {
	return dashRate(effort) * ServerParam::i().dashDirRate(rel_dir);
}

/*-------------------------------------------------------------------*/
/*!

 */
bool PlayerType::normalizeAccel(const Vector2D & velocity,
		const AngleDeg & accel_angle, double * accel_mag) const {
	// if ( *accel_mag > SP.playerAccelMax() )
	//     *accel_mag = SP.playerAccelMax();

	Vector2D dash_move = velocity;
	dash_move += Vector2D::polar2vector(*accel_mag, accel_angle); // add dash accel

	if (dash_move.r2() > playerSpeedMax2() + 0.0001) {
		Vector2D rel_vel = velocity.rotatedVector(-accel_angle);
		// sqr(rel_vel.y) + sqr(max_dash_x) == sqr(max_speed);
		// accel_mag = dash_x - rel_vel.x;
		double max_dash_x = std::sqrt(
				playerSpeedMax2() - rel_vel.y * rel_vel.y);
		*accel_mag = max_dash_x - rel_vel.x;
		return true;
	}
	return false;
}

/*-------------------------------------------------------------------*/
/*!

 */
bool PlayerType::normalizeAccel(const Vector2D & velocity,
		Vector2D * accel) const {
	// double tmp = accel_mag->r();
	// if ( tmp > SP.playerAccelMax() )
	//     *accel *= SP.playerAccelMax() / tmp;

	if ((velocity + (*accel)).r2() > playerSpeedMax2() + 0.0001) {
		Vector2D rel_vel = velocity.rotatedVector(-accel->th());
		// sqr(rel_vel.y) + sqr(max_dash_x) == sqr(max_speed);
		// accel_mag = dash_x - rel_vel.x;
		double max_dash_x = std::sqrt(
				playerSpeedMax2() - rel_vel.y * rel_vel.y);
		accel->setLength(max_dash_x - rel_vel.x);
		return true;
	}
	return false;
}

/*-------------------------------------------------------------------*/
/*!

 */
std::ostream &
PlayerType::print(std::ostream & os) const {
	os << "player_type id : " << id() << "\n  player_speed_max : "
			<< playerSpeedMax() << "\n  stamina_inc_max :  " << staminaIncMax()
			<< "\n  player_decay : " << playerDecay() << "\n  inertia_moment : "
			<< inertiaMoment() << "\n  dash_power_rate : " << dashPowerRate()
			<< "\n  player_size : " << playerSize() << "\n  kickable_margin : "
			<< kickableMargin() << "\n  kick_rand : " << kickRand()
			<< "\n  extra_stamina : " << extraStamina() << "\n  effort_max : "
			<< effortMax() << "\n  effort_min : " << effortMin() << std::endl;
	return os;
}

///////////////////////////////////////////////////////////////////////////////

/*-------------------------------------------------------------------*/
/*!

 */
PlayerTypeSet &
PlayerTypeSet::instance() {
	static PlayerTypeSet S_instance;
	return S_instance;
}

/*-------------------------------------------------------------------*/
/*!

 */
PlayerTypeSet::PlayerTypeSet() {
	resetDefaultType();
}

/*-------------------------------------------------------------------*/
/*!

 */
PlayerTypeSet::~PlayerTypeSet() {

}

/*-------------------------------------------------------------------*/
/*!

 */
void PlayerTypeSet::resetDefaultType() {
	PlayerType default_type;
	insert(default_type);
	M_dummy_type = default_type;
}

/*-------------------------------------------------------------------*/
/*!
 return pointer to param
 */
void PlayerTypeSet::insert(const PlayerType & param) {
	if (static_cast<int>(M_player_type_map.size())
			>= PlayerParam::i().playerTypes()) {
		std::cerr << "# of player type over flow" << std::endl;
		return;
	}

	// insert new type
	M_player_type_map.insert(std::make_pair(param.id(), param));

	if (static_cast<int>(M_player_type_map.size())
			== PlayerParam::i().playerTypes()) {
		createDummyType();
	}
}

/*-------------------------------------------------------------------*/
/*!

 */
void PlayerTypeSet::createDummyType() {
	for (PlayerTypeMap::iterator it = M_player_type_map.begin();
			it != M_player_type_map.end(); ++it) {
		if (it->second.realSpeedMax() > M_dummy_type.realSpeedMax()) {
			M_dummy_type = it->second;
		} else if (std::fabs(
				it->second.realSpeedMax() - M_dummy_type.realSpeedMax()) < 0.01
				&& it->second.cyclesToReachMaxSpeed()
						< M_dummy_type.cyclesToReachMaxSpeed()) {
			M_dummy_type = it->second;
		}
	}
}

/*-------------------------------------------------------------------*/
/*!
 return pointer to param
 */
const PlayerType *
PlayerTypeSet::get(const int id) const {
	if (id == Hetero_Unknown) {
		return &M_dummy_type;
	} else {
		PlayerTypeMap::const_iterator it = M_player_type_map.find(id);
		if (it != M_player_type_map.end()) {
			return &(it->second);
		}
	}

	std::cerr << "PlayerTypeSet: get : player type id error " << id
			<< std::endl;
	return static_cast<PlayerType *>(0);
}

/*-------------------------------------------------------------------*/
/*!

 */
std::ostream &
PlayerTypeSet::print(std::ostream & os) const {
	os << "All Player Types: n";

	PlayerTypeMap::const_iterator it = M_player_type_map.begin();
	while (it != M_player_type_map.end()) {
		it->second.print(os);
		++it;
	}
	return os;
}
}

