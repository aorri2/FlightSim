#pragma once

#include "resource.h"
#include "Obj.h"

#define WS 10000. //##2 10000, Press 's'

enum { MYTEAM = 1, ENTEAM, NOTEAM };
enum { IWORLD, ISEA, ITERRAIN, ISHIP, IFIGHTER, IBULLET, IMINE, NOIMG };

class CIFIGHTER : public CMesh {
public:
	CIFIGHTER() : CMesh(IFIGHTER, vec(0, 0, -1), vec(0, 1, 0), vec(1, 1, 1), vec(0.005, 0.01, 0.0002))
	{
		lift = 0.001F;
	}
	bool virtual TransMtrl(int nMat, mat& mMtrl)
	{
		if (nMat == 5) {	//	for Rotation of propeller
			D3DXMatrixRotationZ(&mMtrl, GetTickCount64() / 20.0f);
			return true;
		}
		return false;
	}
};

class CMine : public CObj {
	DWORD tCreate;
public:
	CMine(CObj Shooter) : CObj(Shooter.Team()) {
		tCreate = timeGetTime();
		Setup(vec(100000, 1, 1), vec(0, 0, -1), vec(0, 1, 0), vec(0.5, 0.5, 0.5), IMINE);
		CObj::Move(Shooter);
		size = 10;

		vec v = up * 0;
		Forced(v);
	}
	void Move() {
		CObj::Move();
		vec v;
		v.x = (rand() % 2001 - 1000) / 10000.F; //-0.1~+0.1 사이의 random값이 나옴
		v.y = (rand() % 2001 - 1000) / 10000.F;
		v.z = (rand() % 2001 - 1000) / 10000.F;
		Forced(v);
		Forced(vec(0, Gravity * 0.9, 0));

		if (timeGetTime() - tCreate > 50000)
			alive = false;
	}
};

class CBullet : public CObj {
	DWORD tCreate;
public:
	CBullet(CObj& Shooter) : CObj(Shooter.Team()) {
		tCreate = timeGetTime();
		Setup(vec(100000, 1, 1), vec(0, 0, -1), vec(0, 1, 0), vec(0.4, 0.4, 0.4), IBULLET);
		CObj::Move(Shooter);
		Forced(10);
		Forced(Shooter.Speed());
		size = 1;
	}
	void Move() {
		CObj::Move();
		if (timeGetTime() - tCreate > 2000)
			alive = false;
	}
};
class CShip : public CObj {
public:
	CShip(int t) : CObj(t) {
	}
	void Move() {
		CObj::Move();
		if (rand() % 100 == 0)	pObjEng->Shoot(*this, IBULLET); //100번에 한번정도
		if (rand() % 500 == 0)	pObjEng->Shoot(*this, IMINE); //500번에 한번정도
	}
};
class CFighter : public CObj
{
public:
	CFighter() : CObj(MYTEAM)
	{

	}
	virtual void Move() {
		if (GetKeyState(VK_UP) & 0x8000) Accel(0.05);
		if (GetKeyState(VK_DOWN) & 0x8000) Break(0.99);
		if (GetKeyState(VK_LEFT) & 0x8000) Yaw(-0.01);
		if (GetKeyState(VK_RIGHT) & 0x8000) Yaw(0.01);
		if (GetKeyState('W') & 0x8000) Pitch(0.01);
		if (GetKeyState('S') & 0x8000) Pitch(-0.01);
		if (GetKeyState('A') & 0x8000) Roll(0.01);
		if (GetKeyState('D') & 0x8000) Roll(-0.01);
		if (GetKeyState(VK_SPACE) & 0x8000) {
			static int FireDelay;
			if (FireDelay++ % 5 == 0) pObjEng->Shoot(*this, IBULLET);
		}
		if (GetKeyState(VK_SHIFT) & 0x8000) {
			static int FireDelay;
			if (FireDelay++ % 5 == 0) pObjEng->Shoot(*this, IMINE);
		}
		if (GetKeyState(VK_ESCAPE) & 0x8000)	PostQuitMessage(0);
		CObj::Move();
		//pObjEng->cam.Move(vec(-100, 0, 0), vec(1, 0, 0), vec(0, 1, 0), 1);
		pObjEng->cam.Move(GetvPos() + GetvFront() * -10 + GetvUp() * 2,
			GetvFront(), 
			 GetvUp(),
			0.5); //for tracing the Fighter

	}


};

class CFlightSim : public CObjEng
{
public : CFlightSim() {}
	void Init(HWND hWnd)
	{
		InitD3D(hWnd);
		InitGeometry();
		InitObject();
	}
	virtual void InitGeometry() 
	{
		const char* saTexName[] = {
			"res\\skybox_top.jpg", "res\\skybox_bottom.jpg",
			"res\\skybox_right.jpg", "res\\skybox_left.jpg",
			"res\\skybox_back.jpg", "res\\skybox_front.jpg",
			0 };
		gmng.Add(new CCube(IWORLD, saTexName, vec(0, 0, 1), vec(0, 1, 0), vec(WS, WS, WS)));
		
		////////////////////////////////////////////////////// Sea
		CVer* pv = new CPlane(ISEA, "res\\skybox_bottom.jpg", vec(0, 0, 1), vec(0, 1, 0), vec(WS, WS, WS));
		gmng.Add(pv);

		////////////////////////////////////////////////////// Terrain
		CMesh* pm = new CMesh(ITERRAIN, vec(0, 0, 1), vec(0, 1, 0), vec(100, 300, 100));
		gmng.Add(pm);
		pm->Load("res\\seafloor.x");

		////////////////////////////////////////////////////// Ship, Airplane
		pm = new CMesh(ISHIP, vec(0, 0, 1), vec(0, 1, 0), vec(1, 1, 1), vec(0.001, 0.001, 0.0001));
		gmng.Add(pm);
		pm->Load("res\\E-TIE-DV.x");

		pm = new CIFIGHTER;
		gmng.Add(pm);
		pm->Load("res\\airplane\\airplane 2.x");

		////////////////////////////////////////////////////// Bullet
		pm = new CMesh(IBULLET, vec(0.F, 1.F, 0.F), vec(0.F, 0.F, 1.F), vec(.03F, .03F, .03F), vec(0.001F, 0.001F, 0.00001F));
		gmng.Add(pm);
		pm->Load("res\\bullet.x");

		////////////////////////////////////////////////////// Mine
		pm = new CMesh(IMINE, vec(0.F, 1.F, 0.F), vec(0.F, 0.F, 1.F), vec(.03F, .03F, .03F), vec(0.001F, 0.001F, 0.001F));
		gmng.Add(pm);
		pm->Load("res\\mine.x");
	}
	virtual void InitObject() {
		int cnt = -1;
		o[++cnt] = new CFighter(); o[cnt]->Setup(vec(0, 30, 0), vec(0, 0, -1), vec(0, 1, 0), vec(1, 1, 1), IFIGHTER);
		o[++cnt] = new CObj(0); o[cnt]->Setup(vec(0, 0, 0), vec(0, 0, -1), vec(0, 1, 0), vec(1, 1, 1), IWORLD);
		o[++cnt] = new CObj(0); o[cnt]->Setup(vec(0, 0, 0), vec(0, 0, -1), vec(0, 1, 0), vec(1, 1, 1), ISEA);			//##1 바다 보이기
		o[++cnt] = new CObj(0); o[cnt]->Setup(vec(0, 0, 0), vec(0, 0, -1), vec(0, 1, 0), vec(1, 1, 1), ITERRAIN);		//##6 땅 보이기
		o[++cnt] = new CShip(ENTEAM); o[cnt]->Setup(vec(-500, 0, 0), vec(0, 0, -1), vec(0, 1, 0), vec(1, 1, 1), ISHIP);		//##6 ...
		o[++cnt] = new CObj(ENTEAM); o[cnt]->Setup(vec(220, 150, 0), vec(1, 0, 0), vec(0, 1, 0), vec(10, 10, 10), IFIGHTER);
		o[++cnt] = new CShip(ENTEAM); o[cnt]->Setup(vec(1000, 0, 0), vec(0, 0, -1), vec(0, 1, 0), vec(1, 1, 1), ISHIP);
	}
	virtual void Shoot(CObj& obj, TImg g)
	{
		int i;
		for (i = 1; i < MAXOBJ && o[i]; i++);	//	Finding Empty Slot
		if (i < MAXOBJ)
			if (g == IBULLET)	o[i] = new CBullet(obj);
			else				o[i] = new CMine(obj);
	}

};