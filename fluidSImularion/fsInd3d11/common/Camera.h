#pragma once
#include "d3dUtil.h"


class Camera
{
public:
	Camera();
	virtual ~Camera() = 0;

	// get camera position
	XMVECTOR GetPositionXM() const;
	XMFLOAT3 GetPosition() const;

	// get camera dimension 
	XMVECTOR GetRightXM() const;
	XMFLOAT3 GetRight() const;
	XMVECTOR GetUpXM() const;
	XMFLOAT3 GetUp() const;
	XMVECTOR GetLookXM() const;
	XMFLOAT3 GetLook() const;

	// get Frustum message 
	float GetNearWindowWidth() const;
	float GetNearWindowHeight() const;
	float GetFarWindowWidth() const;
	float GetFarWindowHeight() const;

	// get view / proj matrix
	XMMATRIX GetViewXM() const;
	XMMATRIX GetProjXM() const;
	XMMATRIX GetViewProjXM() const;

	//orthoView / proj matrix
	XMMATRIX GetBaseViewXM() const;
	XMMATRIX GetOrthoProjXM() const;

	// get viewport
	D3D11_VIEWPORT GetViewPort() const;


	// setFrustum
	void SetFrustum(float fovY, float aspect, float nearZ, float farZ);
	void SetOrthoProj(float screenWidth, float screenHeight, float zn, float zf);
	// setviewport
	void SetViewPort(const D3D11_VIEWPORT& viewPort);
	void SetViewPort(float topLeftX, float topLeftY, float width, float height, float minDepth = 0.0f, float maxDepth = 1.0f);

	// upsate view matrix
	virtual void UpdateViewMatrix() = 0;
	void UpdateBaseViewMatrix();
protected:
	// camera setting 
	XMFLOAT3 m_Position;
	XMFLOAT3 m_Right;
	XMFLOAT3 m_Up;
	XMFLOAT3 m_Look;

	// Frustum attribute
	float m_NearZ;
	float m_FarZ;
	float m_Aspect;
	float m_FovY;
	float m_NearWindowHeight;
	float m_FarWindowHeight;

	// view and projection matrix
	XMFLOAT4X4 m_View;
	XMFLOAT4X4 m_Proj;

	XMFLOAT4X4 m_BaseView;
	XMFLOAT4X4 m_OrthoProj;

	// viewport
	D3D11_VIEWPORT m_ViewPort;

};

class FirstPersonCamera : public Camera
{
public:
	FirstPersonCamera();
	~FirstPersonCamera() override;

	// set camera position
	void SetPosition(float x, float y, float z);
	void SetPosition(const XMFLOAT3& v);
	// set position direction
	void LookAt(FXMVECTOR pos, FXMVECTOR target, FXMVECTOR up);
	void LookAt(const XMFLOAT3& pos, const XMFLOAT3& target, const XMFLOAT3& up);
	void LookTo(FXMVECTOR pos, FXMVECTOR to, FXMVECTOR up);
	void LookTo(const XMFLOAT3& pos, const XMFLOAT3& to, const XMFLOAT3& up);
	// strafe
	void Strafe(float d);
	// walk
	void Walk(float d);
	// forward
	void MoveForward(float d);
	// up/down
	void Pitch(float rad);
	// left/right
	void RotateY(float rad);


	// update view matrix
	void UpdateViewMatrix() override;
};

class ThirdPersonCamera : public Camera
{
public:
	ThirdPersonCamera();
	~ThirdPersonCamera() override;

	// get the target position
	XMFLOAT3 GetTargetPosition() const;
	// get the distance with obj
	float GetDistance() const;
	// get X rotation
	float GetRotationX() const;
	// get Y rotation
	float GetRotationY() const;
	// Rotate vertically around the object(up/down Phi limited in[pi/6, pi/2])
	void RotateX(float rad);
	// rotation around y
	void RotateY(float rad);
	// approach to object
	void Approach(float dist);
	// Set the initial radian around the X axis(up/down Phi limited in[pi/6, pi/2])
	void SetRotationX(float phi);
	// Set the initial radian around the Y axis
	void SetRotationY(float theta);
	// Set and bind the position of the object to be tracked
	void SetTarget(const XMFLOAT3& target);
	// Set initial distance
	void SetDistance(float dist);
	// Set the minimum and maximum allowable distance
	void SetDistanceMinMax(float minDist, float maxDist);
	// update view matrix
	void UpdateViewMatrix() override;

private:
	XMFLOAT3 m_Target;
	float m_Distance;
	// limited min/max distance
	float m_MinDist, m_MaxDist;
	// Based on the world coordinate system, the current rotation angle
	float m_Theta;
	float m_Phi;
};
