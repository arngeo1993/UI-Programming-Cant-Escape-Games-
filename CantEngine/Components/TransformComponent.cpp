
#include "TransformComponent.h"
unsigned const TransformComponent::static_type = BaseComponent::numberOfTypes++;

#include "GameObjects/GameObject.h"
#include "Managers/GameObjectManager.h"
#include "Graphics/D3D11_Renderer.h"


RTTR_REGISTRATION
{
	rttr::registration::class_<TransformComponent>("TransformComponent")
	.constructor<GameObject*>()(rttr::policy::ctor::as_raw_ptr)
	.property("Position", &TransformComponent::m_position)
	.property("Rotation", &TransformComponent::m_rotation)
	.property("Scale", &TransformComponent::m_scale);
}

TransformComponent::TransformComponent(GameObject *owner) :
	BaseComponent(owner, TransformComponent::static_type),
	m_needToRecalculateModel(1),
	m_position(0.f),
	m_rotation(0.f),
	m_scale(1.f),
	m_rotMatrixAutomaticCalc(true)
{
}


TransformComponent::~TransformComponent()
{
}


///////////////////////////////////////////////////////
////           INITIALIZATION METHODS              ////
///////////////////////////////////////////////////////
void TransformComponent::Init(ResourceManager* resMgr, DXRenderer* dxrenderer)
{
}


void TransformComponent::Begin(GameObjectManager *goMgr)
{
}


///////////////////////////////////////////////////////
////           TRANSLATE METHODS                   ////
///////////////////////////////////////////////////////
void TransformComponent::Translate(Vector3 const& translate)
{
	Translate(translate.x, translate.y, translate.z);
}


void TransformComponent::Translate(float dx, float dy, float dz)
{
	m_needToRecalculateModel = 1;

	m_position.x += dx;
	m_position.y += dy;
	m_position.z += dz;
}


///////////////////////////////////////////////////////
////           ROTATE METHODS (DEGREES)            ////
///////////////////////////////////////////////////////
void TransformComponent::Rotate(Vector3 const& rotation)
{
	Rotate(rotation.x, rotation.y, rotation.z);
}

void TransformComponent::Rotate(float eulerDX, float eulerDY, float eulerDZ)
{
	m_needToRecalculateModel = 1;

	m_rotation.x += eulerDX;
	m_rotation.y += eulerDY;
	m_rotation.z += eulerDZ;
}


///////////////////////////////////////////////////////
////           SCALE METHODS                       ////
///////////////////////////////////////////////////////
void TransformComponent::Scale(float val)
{
	Scale(val, val, val);
}


void TransformComponent::Scale(Vector3 const& scale)
{
	Scale(scale.x, scale.y, scale.z);
}


void TransformComponent::Scale(float x, float y, float z)
{
	m_needToRecalculateModel = 1;

	m_scale.x = x;
	m_scale.y = y;
	m_scale.z = z;
}


///////////////////////////////////////////////////////
////    SETTERS METHODS    (Rotation in degrees)   ////
///////////////////////////////////////////////////////


void TransformComponent::SetLocalRotation(float eulerX, float eulerY, float eulerZ)
{
	m_needToRecalculateModel = 1;

	m_rotation.x = eulerX;
	m_rotation.y = eulerY;
	m_rotation.z = eulerZ;
}

void TransformComponent::SetLocalRotationMatrix(const Matrix& rotationMatrix)
{
	m_needToRecalculateModel = 1;
	m_rotMatrix = rotationMatrix;
	m_rotMatrixAutomaticCalc = false;
}

void TransformComponent::SetLocalPosition(const Vector3& position)
{
	SetLocalPosition(position.x, position.y, position.z);
}


void TransformComponent::SetLocalPosition(float x, float y, float z)
{
	m_needToRecalculateModel = 1;
	m_position.x = x;
	m_position.y = y;
	m_position.z = z;
}


///////////////////////////////////////////////////////
////           GETTERS METHODS                     ////
///////////////////////////////////////////////////////
Vector3 TransformComponent::GetForward() const
{
	Vector3 forward;
	forward.x = m_rotMatrix._31;//_13;
	forward.y = m_rotMatrix._32;//_23;
	forward.z = m_rotMatrix._33;//_33;
	return forward;
}

Vector3 TransformComponent::GetRight() const
{
	Vector3 right;
	right.x = m_rotMatrix._11;//_11;
	right.y = m_rotMatrix._12;//_21;
	right.z = m_rotMatrix._13;//_31;
	return right;
}

Vector3 const& TransformComponent::GetPosition() const
{
	return m_position;
}

Vector3 const& TransformComponent::GetRotation() const
{
	return m_rotation;
}

Vector3 const& TransformComponent::GetScale() const
{
	return m_scale;
}

Matrix const& TransformComponent::GetModel() const
{
	return m_model;
}

Matrix const& TransformComponent::GetRotationMatrix() const
{
	return m_rotMatrix;
}

Matrix const& TransformComponent::GetScaleMatrix() const
{
	return m_scaleMatrix;
}

Vector3  TransformComponent::GetPositionNormalized() const
{
	Vector3 result = m_position;
	result.Normalize();
	return result;
}

Vector3  TransformComponent::GetScaleNormalized() const
{
	Vector3 result = m_scale;
	result.Normalize();
	return result;
}


void TransformComponent::SetModel(Matrix const& model) 
{
	this->m_model = model;
}


///////////////////////////////////////////////////////
////           DIRTY FLAG STUFF                    ////
///////////////////////////////////////////////////////
bool TransformComponent::NeedToRecalculate()
{
	return m_needToRecalculateModel;
}

void TransformComponent::ResetDirtyFlag()
{
	m_needToRecalculateModel = false;
}