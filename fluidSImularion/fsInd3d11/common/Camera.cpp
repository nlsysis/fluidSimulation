#include "Camera.h"

Camera::Camera()
	: m_Position(0.0f, 0.0f, 0.0f), m_Right(0.0f, 0.0f, 0.0f),
	m_Up(0.0f, 0.0f, 0.0f), m_Look(0.0f, 0.0f, 0.0f),
	m_NearZ(), m_FarZ(), m_FovY(), m_Aspect(),
	m_NearWindowHeight(), m_FarWindowHeight(),
	m_View(), m_Proj(), m_ViewPort()
{
}

Camera::~Camera()
{
}

/**
	*@brief Get camera position
*/
XMVECTOR Camera::GetPositionXM() const
{
	return XMLoadFloat3(&m_Position);
}

XMFLOAT3 Camera::GetPosition() const
{
	return m_Position;
}

XMVECTOR Camera::GetRightXM() const
{
	return XMLoadFloat3(&m_Right);
}

XMFLOAT3 Camera::GetRight() const
{
	return m_Right;
}

XMVECTOR Camera::GetUpXM() const
{
	return XMLoadFloat3(&m_Up);
}

XMFLOAT3 Camera::GetUp() const
{
	return m_Up;
}

XMVECTOR Camera::GetLookXM() const
{
	return XMLoadFloat3(&m_Look);
}

XMFLOAT3 Camera::GetLook() const
{
	return m_Look;
}

float Camera::GetNearWindowWidth() const
{
	return m_Aspect * m_NearWindowHeight;
}

float Camera::GetNearWindowHeight() const
{
	return m_NearWindowHeight;
}

float Camera::GetFarWindowWidth() const
{
	return m_Aspect * m_FarWindowHeight;
}

float Camera::GetFarWindowHeight() const
{
	return m_FarWindowHeight;
}

XMMATRIX Camera::GetViewXM() const
{
	return XMLoadFloat4x4(&m_View);
}

XMMATRIX Camera::GetProjXM() const
{
	return XMLoadFloat4x4(&m_Proj);
}

XMMATRIX Camera::GetViewProjXM() const
{
	return XMLoadFloat4x4(&m_View) * XMLoadFloat4x4(&m_Proj);
}

XMMATRIX Camera::GetBaseViewXM() const
{
	return XMLoadFloat4x4(&m_BaseView);
}

XMMATRIX Camera::GetOrthoProjXM() const
{
	return XMLoadFloat4x4(&m_OrthoProj);
}

D3D11_VIEWPORT Camera::GetViewPort() const
{
	return m_ViewPort;
}

void Camera::SetFrustum(float fovY, float aspect, float nearZ, float farZ)
{
	m_FovY = fovY;
	m_Aspect = aspect;
	m_NearZ = nearZ;
	m_FarZ = farZ;

	m_NearWindowHeight = 2.0f * m_NearZ * tanf(0.5f * m_FovY);
	m_FarWindowHeight = 2.0f * m_FarZ * tanf(0.5f * m_FovY);

	XMStoreFloat4x4(&m_Proj, XMMatrixPerspectiveFovLH(m_FovY, m_Aspect, m_NearZ, m_FarZ));
}

void Camera::SetOrthoProj(float screenWidth, float screenHeight, float zn, float zf)
{
	XMMATRIX orthographicProj = XMMatrixOrthographicLH(screenWidth, screenHeight, zn, zf);
	XMStoreFloat4x4(&m_OrthoProj, orthographicProj);
}

void Camera::SetViewPort(const D3D11_VIEWPORT & viewPort)
{
	m_ViewPort = viewPort;
}

void Camera::SetViewPort(float topLeftX, float topLeftY, float width, float height, float minDepth, float maxDepth)
{
	m_ViewPort.TopLeftX = topLeftX;
	m_ViewPort.TopLeftY = topLeftY;
	m_ViewPort.Width = width;
	m_ViewPort.Height = height;
	m_ViewPort.MinDepth = minDepth;
	m_ViewPort.MaxDepth = maxDepth;
}

void Camera::UpdateBaseViewMatrix()
{
	XMVECTOR lookat = XMLoadFloat3(&XMFLOAT3(0.0f, 0.0f, 1.0f));
	XMVECTOR up = XMLoadFloat3(&XMFLOAT3(0.0f, 1.0f, 0.0f));
	XMVECTOR position = XMLoadFloat3(&m_Position);

	XMMATRIX view = XMMatrixLookAtLH(position, lookat, up);// D3DXToRadian(degrees));
	XMStoreFloat4x4(&m_BaseView, view);
}


/**
	*@brief FirstPerson camera/free camera
*/

FirstPersonCamera::FirstPersonCamera()
	: Camera()
{
}

FirstPersonCamera::~FirstPersonCamera()
{
}

void FirstPersonCamera::SetPosition(float x, float y, float z)
{
	SetPosition(XMFLOAT3(x, y, z));
}

void FirstPersonCamera::SetPosition(const XMFLOAT3 & v)
{
	m_Position = v;
}

void  FirstPersonCamera::LookAt(FXMVECTOR pos, FXMVECTOR target, FXMVECTOR up)
{
	LookTo(pos, target - pos, up);
}

void FirstPersonCamera::LookAt(const XMFLOAT3 & pos, const XMFLOAT3 & target, const XMFLOAT3 & up)
{
	LookAt(XMLoadFloat3(&pos), XMLoadFloat3(&target), XMLoadFloat3(&up));
}

void FirstPersonCamera::LookTo(FXMVECTOR pos, FXMVECTOR to, FXMVECTOR up)
{
	XMVECTOR L = XMVector3Normalize(to);
	XMVECTOR R = XMVector3Normalize(XMVector3Cross(up, L));
	XMVECTOR U = XMVector3Cross(L, R);

	XMStoreFloat3(&m_Position, pos);
	XMStoreFloat3(&m_Look, L);
	XMStoreFloat3(&m_Right, R);
	XMStoreFloat3(&m_Up, U);
}

void FirstPersonCamera::LookTo(const XMFLOAT3 & pos, const XMFLOAT3 & to, const XMFLOAT3 & up)
{
	LookTo(XMLoadFloat3(&pos), XMLoadFloat3(&to), XMLoadFloat3(&up));
}

void FirstPersonCamera::Strafe(float d)
{
	XMVECTOR Pos = XMLoadFloat3(&m_Position);
	XMVECTOR Right = XMLoadFloat3(&m_Right);
	XMVECTOR Dist = XMVectorReplicate(d);
	/// DestPos = Dist * Right + SrcPos
	XMStoreFloat3(&m_Position, XMVectorMultiplyAdd(Dist, Right, Pos));
}

void FirstPersonCamera::Walk(float d)
{
	XMVECTOR Pos = XMLoadFloat3(&m_Position);
	XMVECTOR Right = XMLoadFloat3(&m_Right);
	XMVECTOR Up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	XMVECTOR Front = XMVector3Normalize(XMVector3Cross(Right, Up));
	XMVECTOR Dist = XMVectorReplicate(d);
	/// DestPos = Dist * Front + SrcPos
	XMStoreFloat3(&m_Position, XMVectorMultiplyAdd(Dist, Front, Pos));
}

void FirstPersonCamera::MoveForward(float d)
{
	XMVECTOR Pos = XMLoadFloat3(&m_Position);
	XMVECTOR Look = XMLoadFloat3(&m_Look);
	XMVECTOR Dist = XMVectorReplicate(d);
	/// DestPos = Dist * Look + SrcPos
	XMStoreFloat3(&m_Position, XMVectorMultiplyAdd(Dist, Look, Pos));
}

void FirstPersonCamera::Pitch(float rad)
{
	XMMATRIX R = XMMatrixRotationAxis(XMLoadFloat3(&m_Right), rad);
	XMVECTOR Up = XMVector3TransformNormal(XMLoadFloat3(&m_Up), R);
	XMVECTOR Look = XMVector3TransformNormal(XMLoadFloat3(&m_Look), R);
	float cosPhi = XMVectorGetY(Look);
	/// upDownPhiviewing angle limited in[2pi/9, 7pi/9]ÅC
	/// cos in [-cos(2pi/9), cos(2pi/9)]
	if (fabs(cosPhi) > cosf(XM_2PI / 9))
		return;

	XMStoreFloat3(&m_Up, Up);
	XMStoreFloat3(&m_Look, Look);
}

void FirstPersonCamera::RotateY(float rad)
{
	XMMATRIX R = XMMatrixRotationY(rad);

	XMStoreFloat3(&m_Right, XMVector3TransformNormal(XMLoadFloat3(&m_Right), R));
	XMStoreFloat3(&m_Up, XMVector3TransformNormal(XMLoadFloat3(&m_Up), R));
	XMStoreFloat3(&m_Look, XMVector3TransformNormal(XMLoadFloat3(&m_Look), R));
}

void FirstPersonCamera::UpdateViewMatrix()
{
	XMVECTOR R = XMLoadFloat3(&m_Right);
	XMVECTOR U = XMLoadFloat3(&m_Up);
	XMVECTOR L = XMLoadFloat3(&m_Look);
	XMVECTOR P = XMLoadFloat3(&m_Position);

	/// normalize verctor
	L = XMVector3Normalize(L);
	U = XMVector3Normalize(XMVector3Cross(L, R));

	/// coss ul for right
	R = XMVector3Cross(U, L);

	/// add view vector
	float x = -XMVectorGetX(XMVector3Dot(P, R));
	float y = -XMVectorGetX(XMVector3Dot(P, U));
	float z = -XMVectorGetX(XMVector3Dot(P, L));

	XMStoreFloat3(&m_Right, R);
	XMStoreFloat3(&m_Up, U);
	XMStoreFloat3(&m_Look, L);

	m_View = {
		m_Right.x, m_Up.x, m_Look.x, 0.0f,
		m_Right.y, m_Up.y, m_Look.y, 0.0f,
		m_Right.z, m_Up.z, m_Look.z, 0.0f,
		x, y, z, 1.0f
	};
	UpdateBaseViewMatrix();
}

/**
	*@brief ThirdPersonCamera
*/

ThirdPersonCamera::ThirdPersonCamera()
	: Camera(), m_Target(), m_Distance(), m_MinDist(), m_MaxDist(), m_Theta(1.5f*MathHelper::Pi), m_Phi(0.25f*MathHelper::Pi)
{
}

ThirdPersonCamera::~ThirdPersonCamera()
{
}

XMFLOAT3 ThirdPersonCamera::GetTargetPosition() const
{
	return m_Target;
}

float ThirdPersonCamera::GetDistance() const
{
	return m_Distance;
}

float ThirdPersonCamera::GetRotationX() const
{
	return m_Phi;
}

float ThirdPersonCamera::GetRotationY() const
{
	return m_Theta;
}

void ThirdPersonCamera::RotateX(float rad)
{
	m_Phi -= rad;
}

void ThirdPersonCamera::RotateY(float rad)
{
	m_Theta = XMScalarModAngle(m_Theta - rad);
}

void ThirdPersonCamera::Approach(float dist)
{
	m_Distance += dist;
	/// limited distance in [m_MinDist, m_MaxDist]
	if (m_Distance < m_MinDist)
		m_Distance = m_MinDist;
	else if (m_Distance > m_MaxDist)
		m_Distance = m_MaxDist;
}

void ThirdPersonCamera::SetRotationX(float phi)
{
	m_Phi = XMScalarModAngle(phi);
	///  upDownPhiviewing angle limited in Phi[pi/6, pi/2]ÅC
	/// cos in [0, cos(pi/6)]
	if (m_Phi < XM_PI / 6)
		m_Phi = XM_PI / 6;
	else if (m_Phi > XM_PIDIV2)
		m_Phi = XM_PIDIV2;
}

void ThirdPersonCamera::SetRotationY(float theta)
{
	m_Theta = XMScalarModAngle(theta);
}

void ThirdPersonCamera::SetTarget(const XMFLOAT3 & target)
{
	m_Target = target;
}

void ThirdPersonCamera::SetDistance(float dist)
{
	m_Distance = dist;
}

void ThirdPersonCamera::SetDistanceMinMax(float minDist, float maxDist)
{
	m_MinDist = minDist;
	m_MaxDist = maxDist;
}

void ThirdPersonCamera::UpdateViewMatrix()
{
	/// sphere coordinate
	float x = m_Target.x + m_Distance * sinf(m_Phi) * cosf(m_Theta);
	float z = m_Target.z + m_Distance * sinf(m_Phi) * sinf(m_Theta);
	float y = m_Target.y + m_Distance * cosf(m_Phi);
	m_Position = { x, y, z };
	XMVECTOR P = XMLoadFloat3(&m_Position);
	XMVECTOR L = XMVector3Normalize(XMLoadFloat3(&m_Target) - P);
	XMVECTOR R = XMVector3Normalize(XMVector3Cross(XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), L));
	XMVECTOR U = XMVector3Cross(L, R);

	/// update vector
	XMStoreFloat3(&m_Right, R);
	XMStoreFloat3(&m_Up, U);
	XMStoreFloat3(&m_Look, L);

	m_View = {
		m_Right.x, m_Up.x, m_Look.x, 0.0f,
		m_Right.y, m_Up.y, m_Look.y, 0.0f,
		m_Right.z, m_Up.z, m_Look.z, 0.0f,
		-XMVectorGetX(XMVector3Dot(P, R)), -XMVectorGetX(XMVector3Dot(P, U)), -XMVectorGetX(XMVector3Dot(P, L)), 1.0f
	};
	UpdateBaseViewMatrix();
}

