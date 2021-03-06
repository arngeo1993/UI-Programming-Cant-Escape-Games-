/**
 * @file TriggerSystem.cpp
 * @author Aleksey Perfilev
 * @date 12/4/2019
 * @copyright Copyright(C) 2019 DigiPen Institute of Technology
 */
 #include "TriggerSystem.h"
#include "../GameObjects/GameObject.h"
#include "Components/TransformComponent.h"
#include "Components/TriggerComponent.h"
#include "Graphics/AppRenderer.h"

unsigned int const TriggerSystem::static_type = BaseSystem::numberOfTypes++;

TriggerSystem::TriggerSystem()
	: BaseSystem()
{
	Push_required_comp<TransformComponent>();
	Push_required_comp<TriggerComponent>();

	EventManager::Get()->SubscribeEvent<GameObjectDestroyed>(this,
		std::bind(&TriggerSystem::OnObjectDeleted, this, std::placeholders::_1));
}

TriggerSystem::~TriggerSystem()
{
	EventManager::Get()->UnsubscribeEvent<GameObjectDestroyed>(this);
}

void TriggerSystem::Register_GameObject(GameObject* go)
{
	TransformComponent* transform = go->GetComponent<TransformComponent>();
	TriggerComponent* trigger = go->GetComponent<TriggerComponent>();

	BaseSystemCompNode* component_node = new TriggerCompNode(transform, trigger);
	component_node->m_goID = go->GetId();

	// register with dynamic aabb
	SpatialPartitionData data1;
	data1.m_Aabb = trigger->m_aabb;
	data1.m_ClientData = trigger;
	m_triggers.InsertData(trigger->m_dynamicAabbTreeKey, data1);

	this->m_ObjComponentsMap[go->GetId()] = component_node;
}

void TriggerSystem::RegisterAppRenderer(AppRenderer* renderer)
{
	m_pAppRenderer = renderer;
}

void TriggerSystem::LateUpdate(float dt)
{
	// updating dynamic aabb tree
	for (auto& node : m_ObjComponentsMap)
	{
		TriggerCompNode* triggerNode = static_cast<TriggerCompNode*>(node.second);
#ifdef DEVELOPER
		if (triggerNode == nullptr)
		{
			DEBUG_LOG("TriggerSystem::LateUpdate(): TriggerNode == nullptr while updating aabb.");
		}
#endif			
		TriggerComponent* trigger = triggerNode->m_trigger;
		TransformComponent* transform = triggerNode->m_transform;
#ifdef DEVELOPER
		if (trigger == nullptr || transform == nullptr)
		{
			DEBUG_LOG("TriggerSystem::LateUpdate(): while updating aabb one or more of the following is a nullptr: trigger, mesh, transform.");
		}
#endif		
		//trigger->UpdateAabb();
		Aabb aabb = trigger->GetAabb();
		const Vector3 translationVec = transform->GetPosition() + trigger->GetOffset();
		Matrix translation = Matrix::CreateTranslation(translationVec);
		//aabb.Transform(translation);		// reconstruct in world space

		aabb.m_Min += translationVec;
		aabb.m_Max += translationVec;


#ifdef DEVELOPER
		if (PhysicsUtils::Settings::isDrawTriggersTree)
		{
			DebugAABBInstance debugAabb;
			debugAabb.m_min_bound = aabb.m_Min;
			debugAabb.m_max_bound = aabb.m_Max;
			debugAabb.m_color = Vector3(0.0f, 1.0f, 0.0f);
			m_pAppRenderer->GetDebugRendering().RegisterDebugAABB(debugAabb);
		}
#endif

		// updating aabb tree
		SpatialPartitionData data(trigger, aabb);
		m_triggers.UpdateData(trigger->m_dynamicAabbTreeKey, data);
	}
	// check for collisions
	QueryResults results;
	m_triggers.SelfQuery(results);
	results.DeleteDuplicates();

	// update triggers
	for (QueryResult& curQuery : results.m_results)
	{
		bool isPrevFound = false;
		for (QueryResult& prevQuery : m_lastFrameQueries.m_results)
		{
			if (curQuery == prevQuery)
			{
				// OnColliding eventfire

				isPrevFound = true;
				break;
			}
		}
		if (!isPrevFound)
		{
			// OnEnter event fire
			TriggerComponent* trigger1 = static_cast<TriggerComponent*>(curQuery.m_clientData0);
			TriggerComponent* trigger2 = static_cast<TriggerComponent*>(curQuery.m_clientData1);

			trigger1->m_onEnter(trigger1->GetOwner(), trigger2->GetOwner());
			trigger2->m_onEnter(trigger2->GetOwner(), trigger1->GetOwner());
		}
	}
	for (QueryResult&  prevQuery : m_lastFrameQueries.m_results)
	{
		bool isCurrentFound = false;
		for (QueryResult& curQuery : results.m_results)
		{
			if (curQuery == prevQuery)
			{
				// OnColliding eventfire

				isCurrentFound = true;
				break;
			}
		}
		if (!isCurrentFound)
		{
			// On exit event fire
			TriggerComponent* trigger1 = static_cast<TriggerComponent*>(prevQuery.m_clientData0);
			TriggerComponent* trigger2 = static_cast<TriggerComponent*>(prevQuery.m_clientData1);

			if (trigger1)
			{
				trigger1->m_onExit(trigger1->GetOwner(), trigger2->GetOwner());
			}
			if (trigger2)
				trigger2->m_onExit(trigger2->GetOwner(), trigger1->GetOwner());
		}
	}

	m_lastFrameQueries = results;
}

const DynamicAabbTree& TriggerSystem::GetTrigersTree()
{
	return m_triggers;
}

void TriggerSystem::OnObjectDeleted(const GameObjectDestroyed* e)
{
	TriggerComponent* trigger = e->m_pGameObject->GetComponent<TriggerComponent>();
	if (trigger)
	{
		m_triggers.RemoveData(trigger->m_dynamicAabbTreeKey);
		//added by albert
		//this fixed the crash when two objects are in trigger and all the sudden
		//the object is destroyed
		for (auto iter = m_lastFrameQueries.m_results.begin(); iter != m_lastFrameQueries.m_results.end();)
		{
			QueryResult& curQuery = *iter;
			if (curQuery.m_clientData0 == trigger || curQuery.m_clientData1 == trigger)
			{
				iter = m_lastFrameQueries.m_results.erase(iter);
			}
			else
			{
				++iter;
			}
		}
	}
}

