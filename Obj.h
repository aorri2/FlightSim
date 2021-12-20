#pragma once

#include "stdlib.h"
#include <D3D9.H>
#include <D3DX9.H>
#include "mmsystem.h"				//	timeGetTime()

#pragma warning(disable : 4244)		//	float -> int
#pragma warning(disable : 4305)		//	double -> float
#pragma warning(disable : 4316)		//	object allocated on the heap may not be aligned 16
#pragma warning(disable : 4996)		//	This function or variable may be unsafe
//#pragma comment(lib, "D3DX9.lib")	// if you want..., Instead of these pragmas, do this, [Project]-[Properties]-[Linker]-[Input]-[Additional Dependencies] : Append these lib ";D3DX9.LIB;D3D9.LIB;WINMM.LIB;dsound.lib;dxguid.lib;"
#pragma comment(lib, "D3D9.lib")
#pragma comment(lib, "WINMM.lib")
#pragma comment(lib, "dsound.lib")
#pragma comment(lib, "dxguid.lib")

typedef D3DXVECTOR3 vec;	// Defining Short Type Name for Vector
typedef D3DXMATRIXA16 mat;	// Defining Short Type Name for Matrix

#define TImg int
#define WX 1024
#define WY 768
#define Gravity 0.002
#define MAXOBJ 100

extern LPDIRECT3DDEVICE9      pDev;	// Device for Drawing

#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZ|D3DFVF_DIFFUSE|D3DFVF_TEX1) // Definition for FVF(Flexible Vertex Format), Vertices use position, color and one texture mapping information
struct CUSTOMVERTEX
{
	FLOAT x, y, z;	// Position of vertex
	DWORD color;	// Color of vertex
	FLOAT u, v;		// Position for texture mapping 
};
struct MYINDEX { WORD _0, _1, _2; };	//	Structure of indices for a triangle's 3 vertices

class CImg {
protected:
	LPDIRECT3DTEXTURE9* pTex;
	int					nTex;
	TImg				gen;
	mat					mCoord;
	double				buoyancy;
	double				lift;
	vec					friction;

public:
	_D3DCULL			CullMode;
	CImg(TImg g, vec front, vec up, vec scale, vec fr) {
		pTex = NULL;
		nTex = 0;
		gen = g;
		buoyancy = Gravity;//0.001;
		friction = fr;
		lift = 0;
		mat mRot, mScale;
		D3DXMatrixLookAtLH(&mRot, &vec(0, 0, 0), &front, &up);
		D3DXMatrixScaling(&mScale, scale.x, scale.y, scale.z);
		mCoord = mRot * mScale;
		CullMode = D3DCULL_CCW;

	}
	~CImg() {
		if (pTex)
		{
			for (int i = 0; i < nTex; i++)
				if (pTex[i]) pTex[i]->Release();
			delete[] pTex;
		}
	}

	virtual void Disp(mat& mWorld) {
		mat m;
		m = mCoord * mWorld;
		pDev->SetTransform(D3DTS_WORLD, &m);
		pDev->SetRenderState(D3DRS_CULLMODE, CullMode);
	};
	friend class CObjEng;
};

class CVer : public CImg {

	LPDIRECT3DVERTEXBUFFER9	pVB;
	LPDIRECT3DINDEXBUFFER9	pIB;
	int* Prim;			//	Number of triangle of each Texture
	DWORD Fvf;
	int nVer, szVer;	//	Number of vertices, Number of byte for a vertex
public:
	CVer(TImg g, vec front, vec up, vec scale, vec fr = vec(10, 10, 10)) : CImg(g, front, up, scale, fr) {
		pVB = NULL;
		pIB = NULL;
		Prim = NULL;
	}

	~CVer() {
		if (pVB) pVB->Release();
		if (pIB) pIB->Release();
		if (Prim) delete Prim;
	}

	bool InitVIBTex(void* vertices, int vsz, int nver, DWORD fvf, /**/ void* indices, int isz, /**/ int* prim, const char** texname) {
		///////////////////////// Vertices
		if (FAILED(pDev->CreateVertexBuffer(vsz, 0, fvf, D3DPOOL_DEFAULT, &pVB, NULL)))
			return false;

		if (vertices) {
			VOID* pVertices;
			if (FAILED(pVB->Lock(0, vsz, (void**)&pVertices, 0)))
				return false;
			memcpy(pVertices, vertices, vsz);
			pVB->Unlock();
			Fvf = fvf;
			nVer = nver;
			szVer = vsz / nver;
		}
		///////////////////////// Indices

		if (FAILED(pDev->CreateIndexBuffer(isz, 0, D3DFMT_INDEX16, D3DPOOL_DEFAULT, &pIB, NULL)))
			return false;

		if (indices) {
			VOID* pIndices;
			if (FAILED(pIB->Lock(0, isz, (void**)&pIndices, 0)))
				return false;
			memcpy(pIndices, indices, isz);
			pIB->Unlock();
		}
		///////////////////////// Texture
		for (nTex = 0; prim[nTex]; nTex++);
		pTex = new LPDIRECT3DTEXTURE9[nTex];
		Prim = new int[nTex];
		memcpy(Prim, prim, sizeof(int) * nTex);
		for (int i = 0; i < nTex; i++) {
			if (texname[i][0] == 0) // texname[i][0] == ""	no texture
				pTex[i] = 0;
			else if (FAILED(D3DXCreateTextureFromFile(pDev, texname[i], &pTex[i])))
				return false;
		}

		return true;
	}

	void Disp(mat& wm) {
		CImg::Disp(wm);
		pDev->SetStreamSource(0, pVB, 0, szVer);
		pDev->SetFVF(Fvf);
		pDev->SetIndices(pIB);
		int i, sum;
		for (i = sum = 0; i < nTex; i++, sum += Prim[i]) {
			if (pTex[i]) {
				pDev->SetTexture(0, pTex[i]);
				pDev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
			}
			else
				pDev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG2);
			pDev->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, nVer, sum * 3, Prim[i]);
		}
	}
};

class CPlane : public CVer {
public:
	CPlane(TImg g, const char* sTexName, vec front, vec up, vec scale, vec fr = vec(10, 10, 10)) : CVer(g, front, up, scale, fr) {
		CullMode = D3DCULL_NONE;
		CUSTOMVERTEX svertices[] =
		{
			{ -1.5,  0,  1.5, 0xff0000ff, 0, 0 },		/// v0
			{  1.5,  0,  1.5, 0xff0000ff, 0, 1 },		/// v1
			{  1.5,  0, -1.5, 0xff0000ff, 1, 1 },		/// v2
			{ -1.5,  0, -1.5, 0xff0000ff, 1, 0 },		/// v3
		};

		MYINDEX	sindices[] =
		{
			{ 0, 1, 2 }, { 0, 2, 3 },
		};

		int sPrim[6] = { 2, 0 };
		const char* saTexName[] = { sTexName, 0 };
		InitVIBTex(svertices, sizeof svertices, sizeof svertices / sizeof(CUSTOMVERTEX), D3DFVF_CUSTOMVERTEX, sindices, sizeof sindices, sPrim, saTexName);
	}
};

class CCube : public CVer {
public:
	CCube(TImg g, const char* saTexName[], vec front, vec up, vec scale, vec fr = vec(10, 10, 10)) : CVer(g, front, up, scale, fr) {
		CullMode = D3DCULL_NONE;
#define V0 -1,  1,  1
#define V1  1,  1,  1
#define V2  1,  1, -1
#define V3 -1,  1, -1
#define V4 -1, -1,  1
#define V5  1, -1,  1
#define V6  1, -1, -1
#define V7 -1, -1, -1

		CUSTOMVERTEX wvertices[] =
		{
			{ V0, 0xffff0000, 0, 1},		// v0 Up Side
			{ V1, 0xff00ff00, 1, 1},		// v1
			{ V2, 0xff0000ff, 1, 0},		// v2
			{ V3, 0xffffff00, 0, 0},		// v3

			{ V5, 0xffff00ff, 1, 0},		// v4 Bottom Side
			{ V4, 0xff00ffff, 0, 0},		// v5
			{ V7, 0xffffffff, 0, 1},		// v6
			{ V6, 0xff000000, 1, 1},		// v7

			{ V2, 0xff00ffff, 1, 0},		// v8 Right Side
			{ V1, 0xffff00ff, 0, 0},		// v9
			{ V5, 0xff000000, 0, 1},		// v10
			{ V6, 0xffffffff, 1, 1},		// v11

			{ V0, 0xff00ffff, 1, 0},		// v12 Left Side
			{ V3, 0xffff00ff, 0, 0},		// v13
			{ V7, 0xff000000, 0, 1},		// v14
			{ V4, 0xffffffff, 1, 1},		// v15

			{ V3, 0xff00ffff, 1, 0},		// v16 Front Side
			{ V2, 0xffff00ff, 0, 0},		// v17
			{ V6, 0xff000000, 0, 1},		// v18
			{ V7, 0xffffffff, 1, 1},		// v19

			{ V1, 0xff00ffff, 1, 0},		// v20 Back Side
			{ V0, 0xffff00ff, 0, 0},		// v21
			{ V4, 0xff000000, 0, 1},		// v22
			{ V5, 0xffffffff, 1, 1},		// v23
		};

		MYINDEX	windices[] =
		{
			{ 0, 2, 1 }, { 0, 3, 2 },	// Up Side
			{ 4, 6, 5 }, { 4, 7, 6 },	// Bottom Side
			{ 8,10, 9 }, { 8,11,10 },	// Left Side
			{12,14,13 }, {12,15,14 },	// Right Side
			{16,18,17 }, {16,19,18 },	// Front Side
			{20,22,21 }, {20,23,22 },	// Back Side
		};
		int Prim[] = { 2, 2, 2, 2, 2, 2, 0 };
		InitVIBTex(wvertices, sizeof wvertices, sizeof wvertices / sizeof(CUSTOMVERTEX), D3DFVF_CUSTOMVERTEX, windices, sizeof windices, Prim, saTexName);
	}
};

class CMesh : public CImg {

	LPD3DXMESH		pMesh;  //	Pointer to Mesh
	D3DMATERIAL9* pMat;	//	Pointer to Materials
	DWORD			nMat;	//	Number of Materials

public:
	CMesh(TImg g, vec front, vec up, vec scale, vec fr = vec(10, 10, 10)) : CImg(g, front, up, scale, fr) {
		pMesh = NULL;
		pMat = NULL;
	}
	~CMesh() {
		if (pMesh) pMesh->Release();
	}
	bool Load(const char* fname) {
		LPD3DXBUFFER pD3DXMtrlBuffer;

		// Read .x file and materials
		D3DXLoadMeshFromX(fname, D3DXMESH_SYSTEMMEM, pDev, NULL, &pD3DXMtrlBuffer, NULL, &nMat, &pMesh);

		// Extracting the information of materials and texture
		D3DXMATERIAL* d3dxMaterials = (D3DXMATERIAL*)pD3DXMtrlBuffer->GetBufferPointer();
		pMat = new D3DMATERIAL9[nMat];
		pTex = new LPDIRECT3DTEXTURE9[nMat];
		nTex = nMat;

		for (DWORD i = 0; i < nMat; i++)
		{
			pMat[i] = d3dxMaterials[i].MatD3D;
			pMat[i].Ambient = pMat[i].Diffuse;

			pTex[i] = NULL;
			if (d3dxMaterials[i].pTextureFilename != NULL && lstrlen(d3dxMaterials[i].pTextureFilename) > 0) {
				char dummy[100], path[100], driver[100];
				_splitpath(fname, driver, path, dummy, dummy);
				strcpy(dummy, driver);
				strcat(dummy, path);
				strcat(dummy, d3dxMaterials[i].pTextureFilename);
				D3DXCreateTextureFromFile(pDev, dummy, &pTex[i]);
			}
		}

		pD3DXMtrlBuffer->Release(); //	release temporary buffer

		return S_OK;
	}

	bool virtual TransMtrl(int nMat, mat& mMtrl) { return false; }

	void Disp(mat& mWorld) {
		CImg::Disp(mWorld);
		for (DWORD i = 0; i < nMat; i++)
		{
			mat mMat;
			if (TransMtrl(i, mMat)) {
				mat mOld, mThis;
				pDev->GetTransform(D3DTS_WORLD, &mOld);
				mThis = mMat * mOld;
				pDev->SetTransform(D3DTS_WORLD, &mThis);

				pDev->SetMaterial(&pMat[i]);
				pDev->SetTexture(0, pTex[i]);
				pMesh->DrawSubset(i);

				pDev->SetTransform(D3DTS_WORLD, &mOld);
				continue;
			}

			if (pTex[i]) {
				pDev->SetTexture(0, pTex[i]);
				pDev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
			}
			else
				pDev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG2);

			pDev->SetMaterial(&pMat[i]);
			pMesh->DrawSubset(i);
		}
	}
};

class CImgManager {

	CImg* Img[100];
	int nGen;

public:
	CImgManager() {
		ZeroMemory(&Img, sizeof(Img));
		nGen = 0;
	}
	~CImgManager() {
		for (int i = 0; i < nGen; i++)
			delete Img[i];
	}

	void Add(CImg* g) { Img[nGen++] = g; }
	void Disp(TImg n, mat& mWorld) { Img[n]->Disp(mWorld); }
	CImg* GeTImg(TImg n) { return Img[n]; }
};

/////////////////////////////////////////////

class CCoord {
protected:

public:
	vec pos, front, up;
	CCoord() : pos(0, 0, 0), front(0, 0, -1)
	{
		up = vec(0, 1, 0);
	}
	void Yaw(double a) {							// near to 0 value +(right), -(left)
		mat m;
		D3DXMatrixRotationAxis(&m, &up, (FLOAT)a);
		D3DXVec3TransformCoord(&front, &front, &m);	//front = front * m;
	}
	void Pitch(double a) {
		vec cross;
		D3DXVec3Cross(&cross, &front, &up);
		mat m;
		D3DXMatrixRotationAxis(&m, &cross, (FLOAT)a);
		D3DXVec3TransformCoord(&front, &front, &m);
		D3DXVec3TransformCoord(&up, &up, &m);
	}
	void Roll(double a) {
		mat m;
		D3DXMatrixRotationAxis(&m, &front, (FLOAT)a);
		D3DXVec3TransformCoord(&up, &up, &m);
	}
	vec& GetvPos() { return pos; }
	vec& GetvFront() { return front; }
	vec& GetvUp() { return up; }
};

class CSpeed : public CCoord {
protected:
public:
	vec speed;
	CSpeed() { speed = vec(0, 0, 0); }
	~CSpeed() { }
	void Move(const vec& p, const vec& f, const vec& u, const vec& s = vec(0, 0, 0)) {
		pos = p, front = f, up = u, speed = s;
	}
	virtual void Move() { pos = pos + speed; }
	void Accel(double a) { speed += front * a; }		// near to 0 and positive value
	void Forced(double a) { speed = front * a; }		// set to an object some value of speed
	void Forced(const vec& f) { speed += f; }
	void Break(double a) { speed *= a; }				// less then 1 and near to 1
	vec& GetvSpeed() { return speed; }
	double GetvVelocity() { return D3DXVec3Length(&speed); }
};

class CCam : public CSpeed {
	FLOAT fovy, Aspect, zn, zf;

public:
	CCam(FLOAT fo = D3DX_PI / 4, FLOAT a = (FLOAT)WX / WY, FLOAT n = 0.4f, FLOAT f = 100000.0f) {
		fovy = fo; Aspect = a; zn = n; zf = f;
	}
	void SetMatrices()
	{
		D3DXVECTOR3 vLookatPt = pos + front;
		D3DXMATRIXA16 matView;
		D3DXMatrixLookAtLH(&matView, &pos, &vLookatPt, &up);
		pDev->SetTransform(D3DTS_VIEW, &matView);

		D3DXMATRIXA16 matProj;
		D3DXMatrixPerspectiveFovLH(&matProj, fovy, Aspect, zn, zf);
		pDev->SetTransform(D3DTS_PROJECTION, &matProj);
	}
	void Move(const vec& p, const vec& f, const vec& u, float ratio)
	{
		pos += (p - pos) * ratio;
		front += (f - front) * ratio;
		up += (u - up) * ratio;		//	delete for cam up vec fixed
	}
	void Move() { pos = pos + speed; }
};

class CObj : public CSpeed {
protected:
public:
	bool alive;
	float size;
	int	team;
	vec scale;
	TImg Img;	// Image Number
	CObj(int t) {
		scale = vec(1, 1, 1);
		speed = vec(0, 0, 0);
		Img = 0;
		alive = true;
		size = 10;
		team = t;
	}
	~CObj() {
	}
	void Setup(vec p, vec f, vec u, vec s, TImg g) {
		D3DXVec3Normalize(&f, &f);
		D3DXVec3Normalize(&u, &u);
		pos = p; front = f; up = u; scale = s; Img = g;
	}
	void Disp();
	virtual void Move() { pos += speed; }
	void Move(CObj& o) { pos = o.pos; front = o.front; up = o.up; speed = o.speed; }
	void Move(const vec& p, const vec& f, const vec& u, const vec& s = vec(0, 0, 0)) {
		pos = p, front = f, up = u, speed = s;
	}
	bool Conflict(CObj* o) {
		if (team && o->team && team != o->team) {
			vec vdist = pos - o->pos;
			float dist = D3DXVec3Length(&vdist);
			if (size + o->size > dist) {
				alive = o->alive = false;
				return true;
			}
		}
		return false;
	};
	bool Alive() { return alive; }
	vec& Speed() { return speed; }
	int	Team() { return team; }

	friend class CObjEng;
};

extern class CObjEng* pObjEng;
class CObjEng {
	LPDIRECT3D9             pD3D = NULL;	// DirectX3D		
public:
	HWND hWnd = NULL;
	CCam cam;
	CObj *o[MAXOBJ];
	//CImg* pImg = 0;
	//CMesh* pMeshImg = 0;
	CImgManager gmng;

	CObjEng() { pObjEng = this; memset(o, 0, sizeof o); }
	HRESULT InitD3D(HWND hWnd)
	{
		CObjEng::hWnd = hWnd;
		if (NULL == (pD3D = Direct3DCreate9(D3D_SDK_VERSION)))
			return E_FAIL;

		D3DPRESENT_PARAMETERS d3dpp;	//	Device descriptor
		ZeroMemory(&d3dpp, sizeof(d3dpp));
		d3dpp.BackBufferCount = 1;
		d3dpp.BackBufferHeight = (UINT)WX;
		d3dpp.BackBufferWidth = (UINT)WY;
		d3dpp.hDeviceWindow = hWnd;
		d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
		d3dpp.BackBufferFormat = D3DFMT_X8R8G8B8;
		d3dpp.EnableAutoDepthStencil = TRUE;
		d3dpp.Windowed = TRUE;
		d3dpp.Flags = 0;
		d3dpp.AutoDepthStencilFormat = D3DFMT_D24S8;
		d3dpp.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;
		d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;

		//	Creating Device(Drawing surface and tools)
		if (FAILED(pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
			hWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING,
			&d3dpp, &pDev)))
		{
			return E_FAIL;
		}
		pDev->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);	// Off Culling
		pDev->SetRenderState(D3DRS_ZENABLE, TRUE);			// Use Z-Buffer

		// About Light
		D3DLIGHT9 g_light;
		ZeroMemory(&g_light, sizeof(D3DLIGHT9));
		g_light.Type = D3DLIGHT_DIRECTIONAL;
		g_light.Diffuse.r = 1.;
		g_light.Diffuse.g = 1.;
		g_light.Diffuse.b = 1.;
		D3DXVec3Normalize((D3DXVECTOR3*)&g_light.Direction, &vec(1, -1, 1));
		g_light.Range = 100000.0;
		pDev->SetLight(0, &g_light);
		pDev->LightEnable(0, TRUE);
		pDev->SetRenderState(D3DRS_LIGHTING, TRUE);
		pDev->SetRenderState(D3DRS_AMBIENT, 0x00202020);
		pDev->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);

		return S_OK;
	}
	virtual void InitGeometry() {}
	virtual void InitObject() {}
	virtual void Cleanup() {}
	virtual void Render()
	{
		// cam.Move(vec(-5, 10, 0), vec(0.5, -1, 0), vec(1, 0, 0), 1);	//	Down look from (0,10,0)
		cam.SetMatrices();

		// Initialize BackBuffer and Z-Buffer
		pDev->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(200, 255, 200), 1.0f, 0);

		// Start of Rendering
		if (SUCCEEDED(pDev->BeginScene()))
		{
			// Rendering Here, Render object to BackBuffer
			mat mWorld;
			D3DXMatrixIdentity(&mWorld);
			static float a = 0, b = 0;
			D3DXMatrixRotationY(&mWorld, a += 0.0001);
			//D3DXMatrixTranslation(&mWorld, a += 0.001, 0, 0);
			//D3DXMatrixScaling(&mWorld, a += 0.001, a, a);
			//pDev->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);	// Off Culling
			//pImg->Disp(mWorld);
			//pMeshImg->Disp(mWorld);
			//gmng.Disp(0, mWorld);
			//gmng.Disp(1, mWorld);
			for (int i = 0; i<MAXOBJ; i++)
				if(o[i])
					o[i]->Disp();
			pDev->EndScene();	// End of Rendering 
		}
		pDev->Present(NULL, NULL, NULL, NULL);	//	Showing Result
	}
	virtual void Physics() {
		//	Gravity, Buoyancy
		for (int i = 0; i < MAXOBJ; i++) {
			if (o[i] == NULL || o[i]->team == 0) continue;

			if (o[i]->pos.y > 0)		o[i]->Forced(vec(0, -Gravity, 0));							// Gravity when on the water surface 
			else if (o[i]->pos.y < 0)	o[i]->Forced(vec(0, gmng.GeTImg(o[i]->Img)->buoyancy, 0));	// Buoyancy when under water

			for (int i = 0; i < MAXOBJ; i++) {
				if (o[i] == NULL) continue;

				//	Friction
				vec cross, fr, fv, vf, vc, vu;
				float f, c, u;
				D3DXVec3Cross(&cross, &o[i]->front, &o[i]->up);
				fr = gmng.GeTImg(o[i]->Img)->friction;
				f = D3DXVec3Dot(&o[i]->speed, &o[i]->front);		// front direction confliction
				c = D3DXVec3Dot(&o[i]->speed, &cross);				// side direction confliction
				u = D3DXVec3Dot(&o[i]->speed, &o[i]->up);			// up and lower direction confliction
				vf = o[i]->front * f * -fr.z;
				vc = cross * c * -fr.x;
				vu = o[i]->up * u * -fr.y;
				vf = vf + vc + vu;
				o[i]->speed += vf;									// power from confliction (confliction each dir * confliction constant each dir)

				//	Lift
				o[i]->speed += o[i]->up * f * gmng.GeTImg(o[i]->Img)->lift;
			}
		}
	}
	virtual void Frame() {
		Physics();

		for (int i = 0; i < MAXOBJ; i++)
			if (o[i]) o[i]->Move();

		for (int i = 0; i < MAXOBJ; i++)
			if (o[i])
				for (int j = 0; j < MAXOBJ; j++)
					if (o[j] && i != j) {
						if (o[i]->Conflict(o[j])) {		//	Confliction test
						}
					}

		for (int i = 0; i < MAXOBJ; i++)
			if (o[i])
				if (!o[i]->Alive()) {
					delete o[i];
					o[i] = NULL;
				}
	}
	virtual void Shoot(CObj& obj, TImg g)
	{
	
	}
};