/* Start Header -------------------------------------------------------
Copyright (C) 2019 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
Primary Author: Jose Rosenbluth
- End Header --------------------------------------------------------*/

#include "BaseSystem.h"
#include "GameObjects/GameObject.h"


unsigned int BaseSystem::numberOfTypes = 0;



BaseSystem::~BaseSystem() 
{
	//Delete the ComponentNodes, but not the components, 
	//since those are deleted by the GameObject
	for (auto& node : m_ObjComponentsMap)
		delete node.second;
	m_ObjComponentsMap.clear();
}


void BaseSystem::UpdateAllNodes(float dt)
{
	for (auto& node : m_ObjComponentsMap)
	{
		this->Update(dt, node.second);
	}
}


void BaseSystem::DrawAllNodes(float dt)
{
	for (auto& node : m_ObjComponentsMap)
	{
		this->Draw(dt, node.second);
	}
}


bool BaseSystem::Can_Register_GameObject(GameObject *go)
{
	//Check for nullptr
	if (go == nullptr)
		return false;

	//Check if the gameobject has the required components
	std::bitset<MAX_NUM_COMPONENTS> compMask = go->GetComponentMask();
	if ((compMask & m_requiredCompMask) != m_requiredCompMask)
		return false;

	//Experiment
	this->Register_GameObject(go);
	return true;
}

void BaseSystem::Unregister_GameObject(size_t go_id)
{
	auto node = m_ObjComponentsMap.find(go_id);
	if (node == m_ObjComponentsMap.end())
		return;
		//Delete node and delete entry from map
	delete node->second;
	m_ObjComponentsMap.erase(go_id);
}


void BaseSystem::Draw(float dt,
	BaseSystemCompNode *compNode) 
{
}