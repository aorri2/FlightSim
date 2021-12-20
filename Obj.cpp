#include "Obj.h"

LPDIRECT3DDEVICE9       pDev = NULL;	// Device for Drawing	
CObjEng* pObjEng = NULL;

void CObj::Disp() 
{
	mat m, mPos, mRot, mScale;
	//D3DXMatrixLookAtLH(&mRot, &vec(0,0,0), &front, &up);
	vec cross;
	D3DXVec3Cross(&cross, &up, &front);
	D3DXMatrixIdentity(&mRot);
	mRot._11 = cross.x; mRot._12 = cross.y; mRot._13 = cross.z;
	mRot._21 = up.x; mRot._22 = up.y; mRot._23 = up.z;
	mRot._31 = front.x; mRot._32 = front.y; mRot._33 = front.z;

	D3DXMatrixScaling(&mScale, scale.x, scale.y, scale.z);
	D3DXMatrixTranslation(&mPos, pos.x, pos.y, pos.z);
	m = mRot * mScale * mPos;
	pObjEng->gmng.Disp(Img, m);
}