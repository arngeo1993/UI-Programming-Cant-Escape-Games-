/**
 * @file TriggerComponent.cpp
 * @author Aleksey Perfilev
 * @date 12/4/2019
 * @copyright Copyright(C) 2019 DigiPen Institute of Technology
 */
#include "TriggerComponent.h"
#include "GameObjects/GameObject.h"
#include "Managers/ResourceManager.h"
#include "Managers/GameObjectManager.h"
#include "Graphics/D3D11_Renderer.h"

unsigned const TriggerComponent::static_type = BaseComponent::numberOfTypes++;

RTTR_REGISTRATION
{
	rttr::registration::class_<TriggerComponent>("TriggerComponent")
		.constructor<GameObject*>()(rttr::policy::ctor::as_raw_ptr)
		.method("Init", &TriggerComponent::Init)
		.property("Scale", &TriggerComponent::m_scale)
		.property("Offset", &TriggerComponent::m_offset)
		.property("CollisionMask", &TriggerComponent::m_collisionMask)
	;
}

TriggerComponent::TriggerComponent(GameObject* owner) 
	: BaseComponent(owner, TriggerComponent::static_type)
	, m_dynamicAabbTreeKey(0)
{
}

TriggerComponent::~TriggerComponent()
{
}

void TriggerComponent::Init(ResourceManager* resMgr, DXRenderer* dxrenderer)
{
	UpdateAabb();
}

void TriggerComponent::Begin(GameObjectManager* goMgr)
{
}

const Aabb& TriggerComponent::GetAabb() const
{
	return m_aabb;
}

const Vector3& TriggerComponent::GetScale()
{
	return m_scale;
}

void TriggerComponent::SetScale(const Vector3& scale)
{
	m_scale = scale;
	UpdateAabb();
}

const Vector3& TriggerComponent::GetOffset()
{
	return m_offset;
}

void TriggerComponent::SetOffset(const Vector3& offset)
{
	m_offset = offset;
}


void TriggerComponent::SetCollisionMask(CollisionTable::CollisionMask mask)
{
	this->m_collisionMask = mask;
}


CollisionTable::CollisionMask TriggerComponent::GetCollisionMask()
{
	return m_collisionMask;
}

void TriggerComponent::UpdateAabb()
{
	m_aabb.m_Max = m_scale * 0.5f;
	m_aabb.m_Min = -m_aabb.m_Max;
}
