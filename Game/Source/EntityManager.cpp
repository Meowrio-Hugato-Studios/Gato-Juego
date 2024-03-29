#include "EntityManager.h"
#include "Player.h"
#include "OwlEnemy.h"
#include "DogEnemy.h"
#include "FurBall.h"
#include "App.h"
#include "Textures.h"
#include "Scene.h"
#include "ScoreItem.h"
#include "FoodItem.h"
#include "Checkpoint.h"

#include "Defs.h"
#include "Log.h"

#ifdef __linux__
#include "External/Optick/include/optick.h"
#elif _MSC_VER
#include "Optick/include/optick.h"
#endif

EntityManager::EntityManager() : Module()
{
	name.Create("entitymanager");
}

EntityManager::EntityManager(bool startEnabled) : Module(startEnabled)
{
	name.Create("entitymanager");
}

// Destructor
EntityManager::~EntityManager()
{}

// Called before render is available
bool EntityManager::Awake(pugi::xml_node& config)
{
	LOG("Loading Entity Manager");
	bool ret = true;

	//Iterates over the entities and calls the Awake
	ListItem<Entity*>* item;
	Entity* pEntity = NULL;

	for (item = entities.start; item != NULL && ret == true; item = item->next)
	{
		pEntity = item->data;

		if (pEntity->active == false) continue;
		ret = item->data->Awake();
	}

	return ret;

}

bool EntityManager::Start() {

	bool ret = true; 

	//Iterates over the entities and calls Start
	ListItem<Entity*>* item;
	Entity* pEntity = NULL;

	for (item = entities.start; item != NULL && ret == true; item = item->next)
	{
		pEntity = item->data;

		if (pEntity->active == false) continue;
		ret = item->data->Start();
	}

	return ret;
}

// Called before quitting
bool EntityManager::CleanUp()
{
	bool ret = true;
	ListItem<Entity*>* item;
	item = entities.end;

	while (item != NULL && ret == true)
	{
		ret = item->data->CleanUp();
		item = item->prev;
	}

	entities.Clear();

	return ret;
}

Entity* EntityManager::CreateEntity(EntityType type)
{
	Entity* entity = nullptr; 

	switch (type)
	{
	case EntityType::PLAYER:
		entity = new Player();
		break;
	case EntityType::OWLENEMY:
		entity = new OwlEnemy();
		break;
	case EntityType::DOGENEMY:
		entity = new DogEnemy();
		break;
	case EntityType::FURBALL:
		entity = new FurBall();
		break;
	case EntityType::SCOREITEM:
		entity = new ScoreItem();
		break;
	case EntityType::FOODITEM:
		entity = new FoodItem();
		break;
	case EntityType::CHECKPOINT:
		entity = new Checkpoint();
		break;
	default:
		break;
	}

	entities.Add(entity);

	return entity;
}

void EntityManager::DestroyEntity(Entity* entity)
{
	ListItem<Entity*>* item;

	for (item = entities.start; item != NULL; item = item->next)
	{
		if (item->data == entity){
			item->data->CleanUp();
			entities.Del(item);
		} 
	}
}

void EntityManager::AddEntity(Entity* entity)
{
	if ( entity != nullptr) entities.Add(entity);
}

bool EntityManager::Update(float dt)
{
	// OPTICK PROFILIN
	OPTICK_EVENT();

	bool ret = true;
	ListItem<Entity*>* item;
	Entity* pEntity = NULL;

	for (item = entities.start; item != NULL && ret == true; item = item->next)
	{
		pEntity = item->data;

		if (pEntity->active == false) continue;
		ret = item->data->Update(dt);
	}

	return ret;
}

bool EntityManager::SaveState(pugi::xml_node node) {
	bool ret = true;
	ListItem<Entity*>* item;
	Entity* pEntity = NULL;

	for (item = entities.start; item != NULL && ret == true; item = item->next)
	{
		pEntity = item->data;

		if (pEntity->active == false) continue;
		
		pEntity->SaveState(node);
	}

	return ret;
}

bool EntityManager::LoadState(pugi::xml_node node)
{
	bool ret = true;
	ListItem<Entity*>* item;
	Entity* pEntity = NULL;

	for (item = entities.start; item != NULL && ret == true; item = item->next)
	{
		pEntity = item->data;

		if (pEntity->active == false) continue;

		pEntity->LoadState(node);
	}

	return ret;
}