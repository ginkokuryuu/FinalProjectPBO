#include "GameWindow.h"
#include "GameFrame.h"
#include "CharOne.h"
#include "CharTwo.h"
#include "MedKit.h"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sstream>
#define TIMER_ID 2000

BEGIN_EVENT_TABLE(GameWindow, wxWindow)
	EVT_PAINT(GameWindow::onPaint)
	EVT_TIMER(TIMER_ID, GameWindow::onTimer)
	EVT_TIMER(TIMER_ID + 1, GameWindow::delayShoot)
	EVT_TIMER(TIMER_ID + 2, GameWindow::enemySpawn)
	EVT_TIMER(TIMER_ID + 3, GameWindow::timeScore)
	EVT_TIMER(TIMER_ID + 4, GameWindow::medSpawn)
	EVT_TIMER(TIMER_ID + 5, GameWindow::bossSpawn)
	EVT_KEY_DOWN(GameWindow::onKeyDown)
	EVT_KEY_UP(GameWindow::onKeyUp)
	EVT_CHAR(GameWindow::onChar)
	EVT_LEFT_DOWN(GameWindow::onClick)
	EVT_MOTION(GameWindow::updateMouse)
END_EVENT_TABLE()

GameWindow::GameWindow(wxFrame * frame)
	: wxWindow(frame, wxID_ANY)
{
	parentWindow = (GameFrame*)frame;
	SetBackgroundStyle(wxBG_STYLE_PAINT);
	SetBackgroundColour(*wxBLACK);
	for (int i = 0; i < 18; i++) {
		for (int j = 0; j < 31; j++) grid[i][j].clear();
	}
	timer = new wxTimer(this, TIMER_ID);
	timer->Start(1); // 1ms refresh delay
	srand(time(NULL)); // generate random seed
	spawner = new wxTimer(this, TIMER_ID + 2);
	spawner->Start(rand() % 4000 + 750); // generate random spawner delay (ms)
	bossspawner = new wxTimer(this, TIMER_ID + 5);
	bossspawner->Start(24000);
	timescore = new wxTimer(this, TIMER_ID + 3);
	timescore->Start(100);
	medspawner = new wxTimer(this, TIMER_ID + 4);
	medspawner->Start(20000);
	player = new CharOne(this, wxGetDisplaySize().GetWidth()/2, wxGetDisplaySize().GetHeight()/2, 25);
	player->setOwner(1);
	if (player->getShieldPtr() != nullptr) (player->getShieldPtr())->setOwner(1);
	updateGrid(player);
	addObject(player);
	imageLoad();

	//ngambil higscore dan highkill
	FILE *text;
	bool state = 1;
	if ((text = fopen("highScore.txt", "r")) == NULL) {
		state = 0;
	}
	if (state) {
		fscanf(text, "%d %d", &highScore, &highKill);
		fclose(text);
	}
	else {
		highScore = 0;
		highKill = 0;
	}
}

GameWindow::~GameWindow()
{
	timer->Stop();
	spawner->Stop();
	timescore->Stop();
	delete timer;
	delete spawner;
	delete timescore;
	delete pausemenu;
	delete medspawner;
	delete bossspawner;
	for (auto it : weapon) delete it;
	delete background;
	delete killcount;
	for (auto it : obj) if (it != nullptr) {
		delete it;
		it = nullptr;
	}
	wxMessageOutputDebug().Printf("over");
}

void GameWindow::onPaint(wxPaintEvent & evt)
{
	wxAutoBufferedPaintDC pdc(this);
	pdc.SetBrush(wxBrush(wxColour(0, 0, 0)));
	pdc.DrawBitmap(*background, wxPoint(0, 0));
	for (auto it : obj) if (it->getObjType() != 3) it->draw(pdc);
	drawUI(pdc);
}

void GameWindow::onTimer(wxTimerEvent & evt)
{
	if (!isPlayerAlive()) {
		timer->Stop();
		spawner->Stop();
		timescore->Stop();
		if (score > highScore) {
			FILE *text;
			highKill = kill;
			highScore = score;
			text = fopen("highScore.txt", "w");
			fprintf(text, "%d %d", highScore, highKill);
			fclose(text);
		}
		parentWindow->GameOver(score, kill, highScore, highKill);
		return;
	}
	for (auto it = obj.begin(); it != obj.end();) {
		(*it)->move();
		updateGrid(*it);
		if ((*it)->getObjType() == 2 && ((*it)->getX() > wxGetDisplaySize().GetWidth() || (*it)->getX() < 0 || (*it)->getY() > wxGetDisplaySize().GetHeight() || (*it)->getY() < 0)) {
			grid[(*it)->getGridY()][(*it)->getGridX()].remove(*it);
			delete *it;
			it = obj.erase(it);
		}
		else ++it;
	}
	checkCollision();
	Refresh(0);
}

void GameWindow::onKeyDown(wxKeyEvent & evt)
{
	if (player != nullptr) {
		int keycode = evt.GetKeyCode();
		//wxMessageOutputDebug().Printf("%d", keycode);
		if (keycode == 27) evt.Skip();
		if (!paused) {
			switch (keycode)
			{
			case 'W':
				player->moveMY();
				break;
			case 'A':
				player->moveMX();
				break;
			case 'S':
				player->moveY();
				break;
			case 'D':
				player->moveX();
				break;
			default:
				break;
			}
		}
	}
}

void GameWindow::onKeyUp(wxKeyEvent & evt)
{
	if (player != nullptr) {
		int keycode = evt.GetKeyCode();
		if (!paused) {
			switch (keycode)
			{
			case 'W':
				player->moveY();
				break;
			case 'A':
				player->moveX();
				break;
			case 'S':
				player->moveMY();
				break;
			case 'D':
				player->moveMX();
				break;
			default:
				break;
			}
		}
	}
}

void GameWindow::onChar(wxKeyEvent & evt)
{
	int key = evt.GetKeyCode();
	if (key == 27) {
		wxMessageOutputDebug().Printf("jiancuk");
		pauseGame();
	}
}

void GameWindow::onClick(wxMouseEvent & evt)
{
	wxPoint mousePos = evt.GetPosition();
	wxMessageOutputDebug().Printf("%d %d", mousePos.x, mousePos.y);
	if (!paused) {
		if (shooter == nullptr && player != nullptr) {
			player->shoot(mousePos.x, mousePos.y);
			shooter = new wxTimer(this, TIMER_ID + 1);
			shooter->StartOnce(200 * player->getWeaponType()); // shoot delay (ms)
		}
	}
	else {
		float scaleY = wxGetDisplaySize().GetHeight() / 1080.0;
		float scaleX = wxGetDisplaySize().GetWidth() / 1920.0;
		if (mousePos.x >= 832*scaleX && mousePos.x <= 1090*scaleX) {
			if (mousePos.y >= 540*scaleY && mousePos.y <= 627*scaleY) pauseGame();
			else if (mousePos.y >= 674*scaleY && mousePos.y <= 755*scaleY) {
				deleteObject(player);
				player = nullptr;
				timer->Stop();
				parentWindow->LoadMenu();
			}
		}
	}
}

void GameWindow::delayShoot(wxTimerEvent & evt)
{
	delete shooter;
	shooter = nullptr;
}

void GameWindow::enemySpawn(wxTimerEvent &evt)
{
	float scaleY = wxGetDisplaySize().GetHeight() / 1080.0;
	float scaleX = wxGetDisplaySize().GetWidth() / 1920.0;
	int randX = rand() % (wxGetDisplaySize().GetWidth()-50) + 25;
	int randY = rand() % (wxGetDisplaySize().GetHeight()-50) + 25;
	int randTime = rand() % abs(10000 - score) + 4000;
	CharOne* enemy = new CharOne(this, randX, randY, 25);
	updateGrid(enemy);
	if (player != nullptr) {
		while ((enemy->getGridX() >= player->getGridX() - 3 && enemy->getGridX() <= player->getGridX() + 3) &&
			(enemy->getGridY() >= player->getGridY() - 3 && enemy->getGridY() <= player->getGridY() + 3)) {
			randX = rand() % 950*scaleX + 50;
			randY = rand() % 650*scaleY + 50;
			enemy->setX(randX);
			enemy->setY(randY);
			updateGrid(enemy);
		}
	}
	addObject(enemy); // spawn enemy on random position
	spawner->Start(randTime); // re-generate random spawner delay(ms)
}

void GameWindow::checkCollision()
{
	for (auto it1 = obj.begin(); it1 != obj.end();) {
		bool hit = 0;
		int gx = (*it1)->getGridX();
		int gy = (*it1)->getGridY();
		for (int y = -1; y < 2; y++) {
			for (int x = -1; x < 2; x++) {
				if (gy + y >= 0 && gy + y < 18 && gx + x >= 0 && gx + x < 31) {
					for (auto it2 = grid[gy + y][gx + x].begin(); it2 != grid[gy + y][gx + x].end();) {
						if ((*it1)->getObjType() == 2) {	// bullet collision
							if ((*it2)->getObjType() == 1) {	// bullet to player collision
								if ((*it1)->isCollidingWith(*it2)) {
									if (((CharOne*)(*it2))->getShieldPtr() != nullptr) {
										if ((*it1)->isCollidingWith(((CharOne*)(*it2))->getShieldPtr())) {
											(((CharOne*)(*it2))->getShieldPtr())->deflect((Bullet*)(*it1));
											wxMessageOutputDebug().Printf("t1");
											++it2;
										}
										else {
											hit = 1;
											if (*it2 == player) {
												if (hp > 1) {
													--hp;
													hitEffect = 255;
													++it2;
												}
												else {
													player = nullptr;
													obj.remove(*it2);
													delete *it2;
													*it2 = nullptr;
													it2 = grid[gy + y][gx + x].erase(it2);
												}
											}
											else {
												obj.remove(*it2);
												delete *it2;
												*it2 = nullptr;
												it2 = grid[gy + y][gx + x].erase(it2);
												score += 100;
												kill++;
												wxMessageOutputDebug().Printf("a1");
											}
											grid[gy + y][gx + x].remove(*it1);
											delete *it1;
											it1 = obj.erase(it1);
											break;
										}
									}
									else {
										hit = 1;
										if (*it2 == player) {
											if (hp > 1) {
												--hp;
												hitEffect = 255;
												++it2;
											}
											else {
												player = nullptr;
												obj.remove(*it2);
												delete *it2;
												*it2 = nullptr;
												it2 = grid[gy + y][gx + x].erase(it2);
											}
										}
										else {
											obj.remove(*it2);
											delete *it2;
											*it2 = nullptr;
											it2 = grid[gy + y][gx + x].erase(it2);
											score += 100;
											kill++;
											wxMessageOutputDebug().Printf("a2");
										}
										grid[gy + y][gx + x].remove(*it1);
										delete *it1;
										it1 = obj.erase(it1);
										break;
									}
								}
								else ++it2;
							}
							else if ((*it2)->getObjType() == 3) {	// bullet to shield collision
								if ((*it1)->isCollidingWith(*it2)) {
									((Shield*)(*it2))->deflect((Bullet*)(*it1));
									wxMessageOutputDebug().Printf("t2");
								}
								++it2;
							}
							else if ((*it2)->getObjType() == 5) {	// bullet to boss collision
								if ((*it1)->isCollidingWith(*it2)) {
									hit = 1;
									if (((CharTwo*)(*it2))->getHP() > 1) {
										((CharTwo*)(*it2))->decHP();
										++it2;
									}
									else {
										obj.remove(*it2);
										delete *it2;
										*it2 = nullptr;
										it2 = grid[gy + y][gx + x].erase(it2);
										score += 1000;
										kill++;
										wxMessageOutputDebug().Printf("a3");
									}
									grid[gy + y][gx + x].remove(*it1);
									delete *it1;
									it1 = obj.erase(it1);
									break;
								}
								else ++it2;
							}
							else ++it2;
						}
						else if ((*it1)->getObjType() == 1){	// player collision
							if ((*it2)->getObjType() == 4) {	// player to medkit collision
								if ((*it1)->isCollidingWith(*it2)) {
									++hp;
									healEffect = 255;
									obj.remove(*it2);
									delete *it2;
									*it2 = nullptr;
									it2 = grid[gy + y][gx + x].erase(it2);
								}
								else ++it2;
							}
							else ++it2;
						}
						else ++it2;
					}
				}
				if (hit) x = 2;
			}
			if (hit) y = 2;
		}
		if (!hit) ++it1;
	}
}


void GameWindow::updateGrid(GameObject * object)
{
	int gx = object->getX() / gridSize;
	int gy = object->getY() / gridSize;
	int tx = object->getGridX();
	int ty = object->getGridY();
	if (gx != tx || gy != ty) {
		grid[ty][tx].remove(object);
		object->setGridX(gridSize);
		object->setGridY(gridSize);
		grid[gy][gx].push_back(object);
	}
}

void GameWindow::addObject(GameObject * object)
{
	obj.push_back(object);
}

void GameWindow::deleteObject(GameObject * object)
{
	grid[object->getGridY()][object->getGridX()].remove(object);
	obj.remove(object);
	delete object;
}

void GameWindow::timeScore(wxTimerEvent & evt)
{
	score++;
}

void GameWindow::drawUI(wxAutoBufferedPaintDC &dc)
{
	wxGraphicsContext *gc = wxGraphicsContext::Create(dc);
	if (isPlayerAlive()) {
		float scaleY = wxGetDisplaySize().GetHeight() / 1080.0;
		float scaleX = wxGetDisplaySize().GetWidth() / 1920.0;
		int curr = player->getWeaponType();
		int next = player->getNextWeapon();
		int dur = player->getWeaponDuration();
		int sw = player->getStopwatchTime();
		double time = (dur - sw) / (double)dur;
		dc.DrawBitmap(wxBitmap(*weapon[curr - 1]), wxPoint(45 * scaleX, 27 * scaleY));
		gc->SetBrush(wxBrush(wxColour(255 * (1 - time), 0, 255 * time, 100)));
		gc->DrawRoundedRectangle(45 * scaleX, 27 * scaleY, 146 * scaleX * (1-time), 146 * scaleY, 20);
		dc.DrawBitmap(wxBitmap(*weapon[next + 2]), wxPoint(45 * scaleX, 220 * scaleY));
		dc.DrawBitmap(wxBitmap(*killcount), wxPoint(320 * scaleX, 118 * scaleY));

		dc.SetFont(wxFont(30 * scaleY, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
		dc.SetTextForeground(wxColour(255, 255, 255));
		std::wstringstream pts;
		pts << score << " pts" << std::ends;
		dc.DrawText(pts.str().c_str(), wxPoint(230 * scaleX, 20 * scaleY));
		std::wstringstream ko;
		ko << kill << std::ends;
		dc.DrawText(ko.str().c_str(), wxPoint(230 * scaleX, 130 * scaleY));

		wxPoint tri[3];
		int s = 20 * scaleY;
		int x = 160 * scaleX;
		int y = 220 * scaleY;
		int dist = 50 * scaleX;
		dc.SetBrush(wxBrush(wxColour(255, 0, 0)));
		for (int i = 0; i < hp; i++) {
			tri[0] = wxPoint(x + dist * i, y);
			tri[1] = wxPoint(x + dist * i + s, y);
			tri[2] = wxPoint(x + dist * i + s / 2, y + s);
			dc.DrawPolygon(3, tri);
		}
		if (hitEffect > 150) {
			gc->SetBrush(wxBrush(wxColour(255, 0, 0, abs((hitEffect -= 10) - 150))));
			gc->DrawRectangle(0, 0, wxGetDisplaySize().GetWidth(), wxGetDisplaySize().GetHeight());
		}
		if (healEffect > 150) {
			gc->SetBrush(wxBrush(wxColour(0, 255, 0, abs((healEffect -= 10) - 150))));
			gc->DrawRectangle(0, 0, wxGetDisplaySize().GetWidth(), wxGetDisplaySize().GetHeight());
		}
	}
	delete gc;
}

void GameWindow::medSpawn(wxTimerEvent & evt)
{
	float scaleY = wxGetDisplaySize().GetHeight() / 1080.0;
	float scaleX = wxGetDisplaySize().GetWidth() / 1920.0;
	int randX = rand() % 950 * scaleX + 50;
	int randY = rand() % 650 * scaleY + 50;
	MedKit* med = new MedKit(randX, randY, this);
	medspawner->Start(rand() % 10000 + 20000);
}

void GameWindow::bossSpawn(wxTimerEvent & evt)
{
	float scaleY = wxGetDisplaySize().GetHeight() / 1080.0;
	float scaleX = wxGetDisplaySize().GetWidth() / 1920.0;
	int randX = rand() % (wxGetDisplaySize().GetWidth() - 120) + 60;
	int randY = rand() % (wxGetDisplaySize().GetHeight() - 120) + 60;
	int randTime = rand() % abs(10000 - score) + 24000;
	CharTwo* boss = new CharTwo(randX, randY, this);
	if (player != nullptr) {
		while ((boss->getGridX() >= player->getGridX() - 3 && boss->getGridX() <= player->getGridX() + 3) &&
			(boss->getGridY() >= player->getGridY() - 3 && boss->getGridY() <= player->getGridY() + 3)) {
			randX = rand() % 950 * scaleX + 50;
			randY = rand() % 650 * scaleY + 50;
			boss->setX(randX);
			boss->setY(randY);
			updateGrid(boss);
		}
	}
	bossspawner->Start(randTime);
}

void GameWindow::imageLoad()
{
	float scaleY = wxGetDisplaySize().GetHeight() / 1080.0;
	float scaleX = wxGetDisplaySize().GetWidth() / 1920.0;
	wxMessageOutputDebug().Printf("x%f", scaleX);
	wxLogNull pls;
	wxStandardPaths &stdPaths = wxStandardPaths::Get();
	wxString execpath = stdPaths.GetExecutablePath();
	wxString filepath = wxFileName(execpath).GetPath() + wxT("\\res\\Pause_Menu.png");
	pausemenu = new wxBitmap(wxImage(filepath).Scale(461 * scaleX, 620 * scaleY));

	filepath = wxFileName(execpath).GetPath() + wxT("\\res\\Kill_Counter.png");
	killcount = new wxBitmap(wxImage(filepath).Scale(67 * scaleX, 53 * scaleY));

	filepath = wxFileName(execpath).GetPath() + wxT("\\res\\Now_Rifle.png");
	weapon[0] = new wxBitmap(wxImage(filepath).Scale(146 * scaleX, 146 * scaleY));

	filepath = wxFileName(execpath).GetPath() + wxT("\\res\\Now_Shotgun.png");
	weapon[1] = new wxBitmap(wxImage(filepath).Scale(146 * scaleX, 146 * scaleY));

	filepath = wxFileName(execpath).GetPath() + wxT("\\res\\Now_Shield.png");
	weapon[2] = new wxBitmap(wxImage(filepath).Scale(146 * scaleX, 146 * scaleY));

	filepath = wxFileName(execpath).GetPath() + wxT("\\res\\Next_Rifle.png");
	weapon[3] = new wxBitmap(wxImage(filepath).Scale(78 * scaleX, 78 * scaleY));

	filepath = wxFileName(execpath).GetPath() + wxT("\\res\\Next_Shotgun.png");
	weapon[4] = new wxBitmap(wxImage(filepath).Scale(78 * scaleX, 78 * scaleY));

	filepath = wxFileName(execpath).GetPath() + wxT("\\res\\Next_Shield.png");
	weapon[5] = new wxBitmap(wxImage(filepath).Scale(78 * scaleX, 78 * scaleY));

	filepath = wxFileName(execpath).GetPath() + wxT("\\res\\Background.png");
	background = new wxBitmap(wxImage(filepath).Scale(wxGetDisplaySize().GetWidth(), wxGetDisplaySize().GetHeight()));
}

void GameWindow::pauseGame()
{
	if (!paused) {
		timer->Stop();
		spawner->Stop();
		timescore->Stop();
		medspawner->Stop();
		bossspawner->Stop();
		for (auto it : obj) {
			it->pause();
		}
		float scaleY = wxGetDisplaySize().GetHeight() / 1080.0;
		float scaleX = wxGetDisplaySize().GetWidth() / 1920.0;
		wxClientDC dc(this);
		dc.DrawBitmap(*pausemenu, wxPoint((wxGetDisplaySize().GetWidth() - 461)*scaleX / 2, (wxGetDisplaySize().GetHeight() - 620)* scaleY / 2));
		paused = true;
	}
	else {
		for (auto it : obj) {
			it->pause();
		}
		timer->Start(1);
		spawner->Start(2000);
		timescore->Start(100);
		medspawner->Start(15000);
		bossspawner->Start(17500);
		paused = false;
		return;
	}
}

bool GameWindow::isPaused()
{
	return paused;
}

int GameWindow::getPlayerX()
{
	if (isPlayerAlive()) return player->getX();
	else return -1;
}

int GameWindow::getPlayerY()
{
	if (isPlayerAlive())  return player->getY();
	else return -1;
}

int GameWindow::getGridSize()
{
	return gridSize;
}

bool GameWindow::isPlayerAlive()
{

	if (player == nullptr) return false;
	else return true;
}

int GameWindow::getMouseX()
{
	return mouseX;
}

int GameWindow::getMouseY()
{
	return mouseY;
}

void GameWindow::updateMouse(wxMouseEvent & evt)
{
	mouseX = evt.GetX();
	mouseY = evt.GetY();
}
