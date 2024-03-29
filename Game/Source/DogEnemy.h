#ifndef __DOGENEMY_H__
#define __DOGENEMY_H__

#include "Animation.h"
#include "Entity.h"
#include "Point.h"
#include "Player.h"
#include "StateMachine.h"
#ifdef __linux__
#include <SDL.h>
#include <Box2D/Box2D.h>
#elif _MSC_VER
#include "SDL/include/SDL.h"
#include "Box2D/Box2D/Box2D.h"
#endif

struct SDL_Texture;

class DogEnemy : public Entity
{
public:

	bool startTimer = true;
	Timer timer;
	Timer movementDelay;

	//EntityState StateMachine(float dt) override;
	void Move(float dt) override;
	void Jump(float dt) override;

	bool SaveState(pugi::xml_node& node) override;
	bool LoadState(pugi::xml_node& node) override;

	DogEnemy();

	virtual ~DogEnemy();

	bool Awake() override;

	bool Start() override;

	bool Update(float dt) override;

	bool CleanUp() override;

	void OnCollision(PhysBody* physA, PhysBody* physB) override;

	void EndCollision(PhysBody* physA, PhysBody* physB) override;

	// Set animations
	void setIdleAnimation();
	void setMoveAnimation();
	void setJumpAnimation();

	void moveToSpawnPoint();
	void pathfindingMovement(float dt);

public:
	//Animations
	Animation idleAnim;
	Animation runAnim;
	Animation hurtAnim;
	Animation dieAnim;
	Animation jumpAnim;
	Animation* currentAnimation = nullptr;

	//Audios
	int dogBark;
	int dogHit;
	int dogAttack;
	int dogDeath;

	int isAlive = true;

	bool canJump = false;

	float maxSpeed = 2.0f;
	const char* texturePath;
	iPoint spawnPosition;
	SDL_Texture* texture = NULL;
	PhysBody* pbody;
	PhysBody* groundSensor;

	SDL_RendererFlip flip = SDL_FLIP_NONE;

	//Movement
	int moveSpeed = 2;
	int angle;
	b2Vec2 movementDirection;
	bool isGrounded = false;

	int currentPathPos;

	Timer attackTimer;
	int lives = 5;
	bool invencible = false;
	Timer reviveTimer;

	StateMachine<DogEnemy>* movementStateMachine;

private:
	Player* player;

};

#endif // __DOGENEMY_H__