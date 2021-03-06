#pragma once
#include "wx\wx.h"
#include "wx\dcbuffer.h"
#include "wx\graphics.h"
#include "GameObject.h"
#include "Bullet.h"
#include "Shield.h"
class Weapon;
class GameWindow;
class CharOne
	: public GameObject
{
private:
	Shield* shield = nullptr;
	Weapon* weapon;
	int a = 5;
	int xDir = 0;
	int yDir = 0;
	int wduration;
	int nextweapon;
	wxTimer *botshooter;
	wxTimer *botmover;
	wxTimer *weaponchanger;
	wxStopWatch stopwatch;
	GameWindow* parent;
	DECLARE_EVENT_TABLE()
public:
	CharOne();
	CharOne(GameWindow* parent, int x, int y, int r);
	bool isCollidingWith(GameObject* o);
	void draw(wxAutoBufferedPaintDC &dc);
	void botShoot(wxTimerEvent &evt);
	void botMove(wxTimerEvent &evt);
	void changeWeapon(wxTimerEvent &evt);
	void shoot(int x, int y);
	void pause();
	void move();
	void moveX();
	void moveY();
	void moveMX();
	void moveMY();
	int getWeaponType();
	int getNextWeapon();
	int getWeaponDuration();
	int getStopwatchTime();
	Shield* getShieldPtr();
	void setShieldPtr(Shield* shield);
	void setShield();
	void deleteShield();
	~CharOne();
};

