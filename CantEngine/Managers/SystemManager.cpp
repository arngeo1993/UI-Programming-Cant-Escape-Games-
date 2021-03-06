/* Start Header -------------------------------------------------------
Copyright (C) 2019 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the 
prior written consent of DigiPen Institute of Technology is prohibited.
Primary Author: Jose Rosenbluth
- End Header --------------------------------------------------------*/


#include "SystemManager.h"
#include "Systems/BaseSystem.h"

//For now, all custom system headers
#include "Systems/CustomSystem/CustomSystem.h"
#include "Systems/RenderingSystem.h"
#include "Systems/RigidbodySystem.h"
#include "Systems/TriggerSystem.h"
#include "Systems/TransformSystem.h"
#include "Systems/TestSystem.h"
#include "Systems/ParticleEmitterSystem.h"
#include "Systems/LightSystem.h"
#include "Systems/AnimationSystem.h"
#include "Systems/CameraSystem.h"
#include "Systems/HaloEffectSystem.h"
#include "Systems/SkyboxIrradianceSystem.h"
#include "Systems/UISystem.h"
#include "Systems/SplineCurvesSystem.h"
#include "Systems/FollowCurvesPathSystem.h"

SystemManager::SystemManager(AppRenderer* appRenderer)
{
	this->AddSystem<CustomSystem>();
	this->AddSystem<FollowCurvesPathSystem>();
	this->AddSystem<SplineCurvesSystem>();
	this->AddSystem<TransformSystem>();
	this->AddSystem<RigidbodySystem>();
	this->AddSystem<TriggerSystem>();
	this->AddSystem<CameraSystem>();
	this->AddSystem<SkyboxIrradianceSystem>();
	this->AddSystem<LightSystem>();
	this->AddSystem<HaloEffectSystem>();
	this->AddSystem<AnimationSystem>();
	this->AddSystem<RenderingSystem>();
	this->AddSystem<ParticleEmitterSystem>();
	this->AddSystem<UISystem>();
	static_cast<LightSystem*>(m_systems[LightSystem::static_type])->RegisterAppRenderer(appRenderer);
	static_cast<HaloEffectSystem*>(m_systems[HaloEffectSystem::static_type])->RegisterAppRenderer(appRenderer);
	static_cast<RenderingSystem*>(m_systems[RenderingSystem::static_type])->RegisterAppRenderer(appRenderer);
	static_cast<RigidbodySystem*>(m_systems[RigidbodySystem::static_type])->RegisterAppRenderer(appRenderer);
	static_cast<TriggerSystem*>(m_systems[TriggerSystem::static_type])->RegisterAppRenderer(appRenderer);
	static_cast<SkyboxIrradianceSystem*>(m_systems[SkyboxIrradianceSystem::static_type])->RegisterAppRenderer(appRenderer);
	static_cast<ParticleEmitterSystem*>(m_systems[ParticleEmitterSystem::static_type])->RegisterAppRenderer(appRenderer);
	static_cast<UISystem*>(m_systems[UISystem::static_type])->RegisterAppRenderer(appRenderer);
	static_cast<SplineCurvesSystem*>(m_systems[SplineCurvesSystem::static_type])->RegisterAppRenderer(appRenderer);
	static_cast<CustomSystem*>(m_systems[CustomSystem::static_type])->RegisterAppRenderer(appRenderer);
}

SystemManager::~SystemManager()
{
	for (auto& node : m_systems)
		delete node.second;
	m_systems.clear();
}


void SystemManager::RegisterGameObject(GameObject *go) 
{
	for (auto& node : m_systems)
	{
		BaseSystem *system = node.second;
		system->Can_Register_GameObject(go);	
	}
}


void SystemManager::Unregister_GameObject(size_t go_id)
{
	for (auto& node : m_systems)
	{
		BaseSystem *system = node.second;
		system->Unregister_GameObject(go_id);
	}
}


void SystemManager::UpdateSystems(float dt)
{
	for (auto& node : m_systems) 
	{
		BaseSystem *system = node.second;

		//Early update (only once per system)
		system->EarlyUpdate(dt);

		// Regular update (once per object registered in the system)
		system->UpdateAllNodes(dt);

		//Late update (only once per system)
		system->LateUpdate(dt);
	}
}


void SystemManager::Draw(float dt)
{
	for (auto& node : m_systems)
	{
		BaseSystem *system = node.second;
		system->DrawAllNodes(dt);
	}
}