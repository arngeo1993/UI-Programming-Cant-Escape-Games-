/* Start Header -------------------------------------------------------
Copyright (C) 2019 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
Primary Author: Jose Rosenbluth
- End Header --------------------------------------------------------*/


#include "GameObject.h"
#include "Managers/GameObjectManager.h"
#include "Factory/Factory.h"

//To call init on each go's components, we need to pass these two
#include "Graphics/AppRenderer.h"
#include "Managers/ResourceManager.h"
#include "Components/AllComponentHeaders.h"


// Initialize static member of class
int GameObject::go_count = 0;

RTTR_REGISTRATION
{
	rttr::registration::class_<GameObject>("GameObject");
}

GameObject::GameObject(GameObjectManager *goMgr, std::string prefabName) :
	m_gameObjectMgr(goMgr), m_id(go_count++), m_compMask(0), m_prefabName(prefabName),
	m_markedForRemoval(false)
{
}

GameObject::GameObject(GameObjectManager *goMgr, std::string prefabName, std::string tag) :
	m_gameObjectMgr(goMgr), m_id(go_count++), m_compMask(0), 
	m_markedForRemoval(false), m_tag(tag), m_prefabName(prefabName)
{
}

GameObject::~GameObject()
{
	//Deleting custom components
	for (auto& node : m_customComponents)
	{
		CantMemory::PoolResource<CustomComponent>::Free(node.second);
	}
	m_customComponents.clear();

	//Deleting engine components
	for (int i = 0; i < MAX_NUM_COMPONENTS; ++i) 
	{
		BaseComponent *c = m_components[i];
		if (c != nullptr)
		{
			if (c->GetType() == TransformComponent::static_type)
				CantMemory::PoolResource<TransformComponent>::Free(static_cast<TransformComponent*>(c));
			
			else if (c->GetType() == RigidbodyComponent::static_type)
				CantMemory::PoolResource<RigidbodyComponent>::Free(static_cast<RigidbodyComponent*>(c));
			
			else if (c->GetType() == RendererComponent::static_type)
				CantMemory::PoolResource<RendererComponent>::Free(static_cast<RendererComponent*>(c));
			
			else if (c->GetType() == MeshComponent::static_type)
				CantMemory::PoolResource<MeshComponent>::Free(static_cast<MeshComponent*>(c));
			
			else if (c->GetType() == LightComponent::static_type)
				CantMemory::PoolResource<LightComponent>::Free(static_cast<LightComponent*>(c));
			
			else if (c->GetType() == CameraComponent::static_type)
				CantMemory::PoolResource<CameraComponent>::Free(static_cast<CameraComponent*>(c));
			
			else if (c->GetType() == ParticleEmitterComponent::static_type)
				CantMemory::PoolResource<ParticleEmitterComponent>::Free(static_cast<ParticleEmitterComponent*>(c));
			
			else if (c->GetType() == HaloEffectComponent::static_type)
				CantMemory::PoolResource<HaloEffectComponent>::Free(static_cast<HaloEffectComponent*>(c));
			
			else if (c->GetType() == UIComponent::static_type)
				CantMemory::PoolResource<UIComponent>::Free(static_cast<UIComponent*>(c));
			
			else if (c->GetType() == AnimationComponent::static_type)
				CantMemory::PoolResource<AnimationComponent>::Free(static_cast<AnimationComponent*>(c));
			
			else if (c->GetType() == SkyboxIrradianceComponent::static_type)
				CantMemory::PoolResource<SkyboxIrradianceComponent>::Free(static_cast<SkyboxIrradianceComponent*>(c));

			else if (c->GetType() == TriggerComponent::static_type)
				CantMemory::PoolResource<TriggerComponent>::Free(static_cast<TriggerComponent*>(c));

			else if (c->GetType() == FollowCurvesPathComponent::static_type)
				CantMemory::PoolResource<FollowCurvesPathComponent>::Free(static_cast<FollowCurvesPathComponent*>(c));

			else if (c->GetType() == SplineCurvesComponent::static_type)
				CantMemory::PoolResource<SplineCurvesComponent>::Free(static_cast<SplineCurvesComponent*>(c));
		}
	}
}

size_t GameObject::GetId() const
{
	return this->m_id;
}

GameObject::ComponentMask GameObject::GetComponentMask() const
{
	return m_compMask;
}

const std::string& GameObject::GetTag() const
{
	return this->m_tag;
}

void GameObject::Destroy()
{
	this->m_markedForRemoval = true;

	//Stuff that must happen when a gameobject is marked for remove

	// 1- Communicate to the GO MGR so the go is queued for deletion
	// TODO - Change for event call
	m_gameObjectMgr->Queue_GameObject_Destruction(this->GetId());
}

const std::string& GameObject::GetPrefabName() const
{
	return m_prefabName;
}

bool GameObject::Is_marked_for_remove() const
{
	return m_markedForRemoval;
}

void GameObject::Begin()
{
	//Engine components begin call
	for (int i = 0; i < MAX_NUM_COMPONENTS; ++i)
	{
		BaseComponent *c = m_components[i];
		if (c)
			c->Begin(m_gameObjectMgr);
	}

	//Scripted begin call
	for (auto& node : m_customComponents) 
	{
		if (node.second) //TODO - figure out why this being filled even with null
			node.second->Begin(m_gameObjectMgr);
	}
}


void GameObject::Init(AppRenderer *pRenderer, ResourceManager *pResMgr)
{
	//Engine components init call
	for (int i = 0; i < MAX_NUM_COMPONENTS; ++i)
	{
		BaseComponent *c = m_components[i];
		if (c)
			c->Init(pResMgr, pRenderer->GetDXRenderer());
	}

	//Scripted begin call
	for (auto& node : m_customComponents)
	{
		if (node.second) //TODO - figure out why this being filled even with null
			node.second->Init(pResMgr, pRenderer->GetDXRenderer());
	}
}


CustomComponent *GameObject::AddCustomComponent(const std::string& scriptName)//, ScriptingManager *luaMgr)
{
	//Get the correct script name from the path
	size_t index = scriptName.find_last_of("/\\");
	size_t len = (scriptName.size() - 4) - (index + 1);
	std::string name = scriptName.substr(index + 1, len);

	//Check first if the gameobj already has this custom component
	CustomComponent *component = GetCustomComponent(name); // m_customComponents is per object
	if (component)
		return component;

	//If it does not exist, create it and add it
	component = CantMemory::PoolResource<CustomComponent>::Allocate(this);			

	//If it was created correctly
	if (component)
	{
		component->ScriptSetup(scriptName, name, this->m_gameObjectMgr->GetScriptingManager());
		m_customComponents[name] = component;
		return component;
	}
	return nullptr;
}


CustomComponent *GameObject::GetCustomComponent(std::string scriptName)
{
	auto it = m_customComponents.find(scriptName);
	if (it != m_customComponents.end())
		return it->second;
	return nullptr;
}


sol::table const& GameObject::LuaGetCustomComponent(std::string scriptName) 
{
	CustomComponent *comp = GetCustomComponent(scriptName);
	if (comp)
	{
		return comp->getCustomCompLuaRef();
	}
	return  refHolder;
}


sol::table GameObject::LuaAddCustomComponent(std::string scriptName)
{
	//First see if it already exist on the gameobj customComponents
	CustomComponent *comp = GetCustomComponent(scriptName);
	if (comp)
		return comp->getCustomCompLuaRef();

	//If it does not exist, create it and add it
	comp = CantMemory::PoolResource<CustomComponent>::Allocate(this);

	//If it was created correctly
	if (comp)
	{
		std::string wholePath = "Scripts\\Components\\" + scriptName + ".lua";
		comp->ScriptSetup(wholePath, scriptName, this->m_gameObjectMgr->GetScriptingManager());
		m_customComponents[scriptName] = comp;
		return comp->getCustomCompLuaRef();
	}

	//If creation failed, return this empty table ref
	return  refHolder;
}


GameObjectManager *GameObject::GetGOManager() 
{
	return this->m_gameObjectMgr;
}


//////////////////////////////////////////////////////////////
////             INSTANTIATION                            ////
//////////////////////////////////////////////////////////////

GameObject *GameObject::Instantiate(GameObjectManager *goMgr)
{
	GameObject *go = CantMemory::PoolResource<GameObject>::Allocate(goMgr, "");
	if (go == nullptr)
		return go;

	//Add to the goMgr's list of scripted Intantiations
	goMgr->AddToScriptInstantiateQueue(go);
	
	//Return the pointer
	return go;
}

GameObject *GameObject::Instantiate(GameObjectManager *goMgr, std::string const& prefabPath)
{
	GameObject* go = CantMemory::PoolResource<GameObject>::Allocate(goMgr, prefabPath);
	Factory::LoadObject(go, prefabPath);
	//Add to the goMgr's list of scripted Intantiations
	goMgr->AddToScriptInstantiateQueue(go);
	return go;
}