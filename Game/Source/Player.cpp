#include "Player.h"
#include "App.h"
#include "Entity.h"
#include "FadeToBlack.h"
#include "Map.h"
#include "StateMachine.h"
#include "States/Player/PlayerDeadState.h"
#include "States/Player/PlayerHurtState.h"
#include "States/Player/PlayerNoClipState.h"
#include "States/Player/PlayerWinState.h"
#include "Textures.h"
#include "Audio.h"
#include "Input.h"
#include "Render.h"
#include "Scene.h"
#include "Log.h"
#include "Point.h"
#include "Physics.h"
#include "FurBall.h"

//States
#include "States/Player/PlayerIdleState.h"
#include "States/Player/PlayerMoveState.h"
#include "States/Player/PlayerClimbState.h"

#include <cmath>
#include <iostream>

#ifdef __linux__
#include <Box2D/Common/b2Math.h>
#include <Box2D/Dynamics/b2Body.h>
#include <Box2D/Dynamics/b2Fixture.h>
#endif

Player::Player() : Entity(EntityType::PLAYER)
{
	name.Create("Player");
}

Player::~Player() {

}

bool Player::Awake() {

	position.x = parameters.attribute("x").as_int();
	position.y = parameters.attribute("y").as_int();
	texturePath = parameters.attribute("texturepath").as_string();
	spawnPosition = position;
	
	return true;
}

bool Player::Start() {

	timer = Timer();
	shootCooldown = Timer(5);

	//load Animations TODO: identify animations by name (en teoria ya esta hecho pero hay que hacer la funcion que te devuelve la animacion por nombre)
	walkAnim = *app->map->GetAnimByName("Cat-1-Walk");
	walkAnim.speed = 8.0f;
	idleAnim = *app->map->GetAnimByName("Cat-1-Idle");
	idleAnim.speed = 8.0f;
	jumpAnim = *app->map->GetAnimByName("Cat-1-Run");
	jumpAnim.speed = 8.0f;
	hurtAnim = *app->map->GetAnimByName("Cat-1-Hurt");
	hurtAnim.speed = 16.0f;
	hurtAnim.loop = false;

	currentAnimation = &idleAnim;

	//pbody = app->physics->CreateCircle(position.x + 16, position.y + 16, 16, bodyType::DYNAMIC);
	pbody = app->physics->CreateRectangle(position.x, position.y, 20, 10, bodyType::DYNAMIC);
	pbody->listener = this;
	pbody->ctype = ColliderType::PLAYER;

	//si quieres dar vueltos como la helice de un helicoptero Boeing AH-64 Apache pon en false la siguiente funcion
	pbody->body->SetFixedRotation(true);
	pbody->body->GetFixtureList()->SetFriction(25.0f);
	pbody->body->SetLinearDamping(1);

	// Create player sensors
	groundSensor = app->physics->CreateRectangleSensor(position.x, position.y + pbody->width, 10, 5, bodyType::DYNAMIC);
	groundSensor->listener = this;

	topSensor = app->physics->CreateRectangleSensor(position.x, position.y + pbody->width, 10, 1, bodyType::DYNAMIC);
	topSensor->listener = this;

	leftSensor = app->physics->CreateRectangleSensor(position.x, position.y + pbody->width, 1, 5, bodyType::DYNAMIC);
	leftSensor->listener = this;

	rightSensor = app->physics->CreateRectangleSensor(position.x, position.y + pbody->width, 1, 5, bodyType::DYNAMIC);
	rightSensor->listener = this;
	
	// Load audios
	playerAttack = app->audio->LoadFx("Assets/Audio/Fx/CatAttack.wav");
	playerAttack2 = app->audio->LoadFx("Assets/Audio/Fx/CatAttack2.wav");
	playerDeath = app->audio->LoadFx("Assets/Audio/Fx/CatDeath.wav");
	playerHit = app->audio->LoadFx("Assets/Audio/Fx/CatHit.wav");
	playerJump = app->audio->LoadFx("Assets/Audio/Fx/CatJump.wav");
	playerWalk = app->audio->LoadFx("Assets/Audio/Fx/CatWalk.ogg");
	playerMeow = app->audio->LoadFx("Assets/Audio/Fx/CatMeow.wav");
	playerWin = app->audio->LoadFx("Assets/Audio/Fx/Win.ogg");
	pickItem = app->audio->LoadFx("Assets/Audio/Fx/PickItem.wav");

	raycastTest = app->physics->CreateRaycast(this, pbody->body->GetPosition(), {pbody->body->GetPosition().x, pbody->body->GetPosition().y + 0.4f});

	// Create player state machine
	stateMachineTest = new StateMachine<Player>(this);
	stateMachineTest->AddState(new PlayerIdleState("idle"));
	stateMachineTest->AddState(new PlayerMoveState("move"));
	stateMachineTest->AddState(new PlayerClimbState("climb"));
	stateMachineTest->AddState(new PlayerHurtState("hurt"));
	stateMachineTest->AddState(new PlayerDeadState("dead"));
	stateMachineTest->AddState(new PlayerWinState("win"));
	stateMachineTest->AddState(new PlayerNoClipState("noclip"));

	// TODO load debug menu texture from xml
	// load debug menu texture
	debugMenuTexture = app->tex->Load("Assets/Textures/debug_menu.png");

	return true;
}

bool Player::Update(float dt)
{
	LOG("%d", score);
	stateMachineTest->Update(dt);

	debugTools();

 	if (app->input->GetMouseButtonDown(SDL_BUTTON_LEFT) == KEY_DOWN)
	{
		if (shootCooldown.ReadMSec() > shootCooldownTime)
		{

			// AUDIO DONE player attack
			app->audio->PlayFx(playerAttack);
			app->audio->PlayFx(playerAttack2);

			b2Vec2 mouseWorldPosition = { PIXEL_TO_METERS(app->input->GetMouseX()) + PIXEL_TO_METERS(-app->render->camera.x), PIXEL_TO_METERS(app->input->GetMouseY()) + PIXEL_TO_METERS(-app->render->camera.y) };
			b2Vec2 shootDir = {mouseWorldPosition - pbody->body->GetPosition()};
			shootDir.Normalize();

			FurBall* bullet = (FurBall*)app->entityManager->CreateEntity(EntityType::FURBALL);
			bullet->Awake();
			bullet->Start();

			bullet->pbody->body->SetTransform(pbody->body->GetPosition() + b2Vec2{0, 0}, 0);
			bullet->pbody->body->SetAwake(false);
			bullet->pbody->body->ApplyForce({ shootDir.x * bulletSpeed, shootDir.y * bulletSpeed}, bullet->pbody->body->GetWorldCenter(), true);
			shootCooldown.Start();
		}
	}
	//debug shootDir
	if(debug)
		{
			b2Vec2 mouseWorldPosition = { PIXEL_TO_METERS(app->input->GetMouseX()) + PIXEL_TO_METERS(-app->render->camera.x), PIXEL_TO_METERS(app->input->GetMouseY()) + PIXEL_TO_METERS(-app->render->camera.y) };
			app->render->DrawLine(METERS_TO_PIXELS(pbody->body->GetPosition().x), METERS_TO_PIXELS(pbody->body->GetPosition().y), METERS_TO_PIXELS(mouseWorldPosition.x), METERS_TO_PIXELS(mouseWorldPosition.y), 255, 0, 0);
		}

	// Update player state

	/* if(state == EntityState::MOVE) {
		if (isCollidingLeft or isCollidingRight and !isGrounded) {
			state = EntityState::CLIMB;
		}
	} */

	//StateMachine(dt);
	//LOG("state: %d", state);

	pbody->body->SetTransform(pbody->body->GetPosition(), angle*DEGTORAD);

	//Update player position in pixels
	position.x = METERS_TO_PIXELS(pbody->body->GetTransform().p.x) - 16;
	position.y = METERS_TO_PIXELS(pbody->body->GetTransform().p.y) - 16;

	//Update Raycast position
	raycastTest->rayStart = pbody->body->GetPosition();
	float32 rotatedX = pbody->body->GetPosition().x + 0.4f * SDL_cos(pbody->body->GetAngle() + DEGTORAD * 90);
	float32 rotatedY = pbody->body->GetPosition().y + 0.4f * SDL_sin(pbody->body->GetAngle() + DEGTORAD * 90);
	raycastTest->rayEnd = { rotatedX, rotatedY };

	// Update player sensors
	CopyParentRotation(pbody, groundSensor, -12, -2, 270);

	CopyParentRotation(pbody, topSensor, -14, 0, 90);

	CopyParentRotation(pbody, leftSensor, 0, 1, 0);

	CopyParentRotation(pbody, rightSensor, 0, 1, 180);	
	
	//SDL_Rect rect = { 0,0,50,50 };
	app->render->DrawTexture(currentAnimation->texture, position.x - 9, position.y - 9, &currentAnimation->GetCurrentFrame(), 1.0f, pbody->body->GetAngle()*RADTODEG, flip);

	currentAnimation->Update(dt);

	//REMOVE
	app->render->DrawRectangle({METERS_TO_PIXELS(pointTest.x) - 1, METERS_TO_PIXELS(pointTest.y) - 1, 2,2}, 0, 0, 255);
	app->render->DrawLine(METERS_TO_PIXELS(pointTest.x), METERS_TO_PIXELS(pointTest.y), METERS_TO_PIXELS(pointTest.x + (normalTest.x * 10)), METERS_TO_PIXELS(pointTest.y + (normalTest.y * 10)), 255, 255, 0);

	return true;
}

void Player::debugTools()
{
	// DEBUG TOOLS ------------------------------------------------

	if (debug) {
		// Draw debug menu

		//app->render->DrawTexture(debugMenuTexture, position.x, position.y);
		//app->render->DrawTexture(debugMenuTexture, app->render->camera.x, app->render->camera.y);
	}

	// Toggle on/off debug mode + View colliders / logic
	if (app->input->GetKey(SDL_SCANCODE_F1) == KEY_DOWN) {
		debug = !debug;
		
	}

	if (debug) {
		// Teleport Menu Level
		if (app->input->GetKey(SDL_SCANCODE_F2) == KEY_DOWN) {
			// Resetart Level 1
			moveToSpawnPoint();
			// Resetart Level 2
			// TODO teleport player to the spawnPosition of the next level
			// position = spawnPosition;
		}

		// Restart current level (Kill player)
		if (app->input->GetKey(SDL_SCANCODE_F3) == KEY_DOWN) {
			lives = 0;
			isAlive = false;
		}

		// Toggle free cam mode on/off
		if (app->input->GetKey(SDL_SCANCODE_F4) == KEY_DOWN) {
			freeCam = !freeCam;
		}
		// Save game state
		if (app->input->GetKey(SDL_SCANCODE_F5) == KEY_DOWN) app->SaveRequest();

		// Load game state
		if (app->input->GetKey(SDL_SCANCODE_F6) == KEY_DOWN) app->LoadRequest();

		// Toggle god mode on/off
		if (app->input->GetKey(SDL_SCANCODE_F7) == KEY_DOWN) {
			godMode = !godMode;
		}

		// Toggle no-clip mode on/off
		if (app->input->GetKey(SDL_SCANCODE_F10) == KEY_DOWN) {
			noClip = !noClip;
			//state = EntityState::NO_CLIP;
			stateMachineTest->ChangeState("noclip");
			this->pbody->body->GetFixtureList()->SetSensor(true);
		}

		// Toggle FPS cap on/off
		if (app->input->GetKey(SDL_SCANCODE_F11) == KEY_DOWN) {
			fpsLimiter = !fpsLimiter;
		}
	}

	// END OF DEBUG TOOLS -----------------------------------------
}

void Player::setIdleAnimation()
{
	currentAnimation = &idleAnim;
}

void Player::setMoveAnimation()
{
	currentAnimation = &walkAnim;
	jumpAnim.Reset();
}

void Player::setJumpAnimation()
{
	currentAnimation = &jumpAnim;
}

void Player::setClimbAnimation()
{
	currentAnimation = &walkAnim;
}

void Player::setWinAnimation()
{
	// TODO set win animation
}


void Player::Move(float dt) {

	// AUDIO DONE player walk

	if (playerWalkSound.ReadMSec() > 245 and isGrounded)
	{
		app->audio->PlayFx(playerWalk);
		playerWalkSound.Start();
	}
	if (app->input->GetKey(SDL_SCANCODE_A) == KEY_REPEAT) {

		if (pbody->body->GetLinearVelocity().x >= -maxSpeed)
		{
			float impulse = pbody->body->GetMass() * moveForce;
			pbody->body->ApplyLinearImpulse({ impulse * (float32)SDL_cos(pbody->body->GetAngle() + DEGTORAD * 180), impulse * (float32)SDL_sin(pbody->body->GetAngle() + DEGTORAD * 180) }, pbody->body->GetWorldCenter(), true);
		}
		flip = SDL_FLIP_HORIZONTAL;
	}

	else if (app->input->GetKey(SDL_SCANCODE_D) == KEY_REPEAT) {
		if (pbody->body->GetLinearVelocity().x <= maxSpeed)
		{
			float impulse = pbody->body->GetMass() * moveForce;
			pbody->body->ApplyLinearImpulse({ impulse * (float32)SDL_cos(pbody->body->GetAngle()), impulse * (float32)SDL_sin(pbody->body->GetAngle()) }, pbody->body->GetWorldCenter(), true);
		}
		flip = SDL_FLIP_NONE;

	}
	/* else if (isGrounded) {
		state = EntityState::IDLE;
	} */

}

void Player::Jump(float dt) {

	// AUDIO DONE player jump
	app->audio->PlayFx(playerJump);

	float impulse = pbody->body->GetMass() * 5;
	pbody->body->ApplyLinearImpulse(b2Vec2(0, -impulse), pbody->body->GetWorldCenter(), true);
	isGrounded = false;

}

void Player::Climb(float dt) {


	// AUDIO DONE player walk (climb)
	if (playerWalkSound.ReadMSec() > 245 and isGrounded)
	{
		app->audio->PlayFx(playerWalk);
		playerWalkSound.Start();
	}

	if (startTimer) {
		timer.Start();
		startTimer = false;
	}

	LOG("TIMER: %d", timer.ReadSec());

	if (timer.ReadSec() <= 5) {
		if (isCollidingRight) {
			climbingLeft = false;
			climbingRight = true;
			flip = SDL_FLIP_NONE;
		}

		else if (isCollidingLeft) {
			climbingRight = false;
			climbingLeft = true;
			flip = SDL_FLIP_HORIZONTAL;
		}

		if (climbingRight) {
			angle = std::lerp(angle, -90, dt * 32 / 1000);
			pbody->body->ApplyForceToCenter({ 1, 0 }, true);
		}
		else if (climbingLeft) {
			angle = std::lerp(angle, 90, dt * 32 / 1000);
			pbody->body->ApplyForceToCenter({ -1, 0 }, true);
		}

		pbody->body->ApplyForceToCenter({ 0, -2.0f }, true);

		if (app->input->GetKey(SDL_SCANCODE_W) == KEY_REPEAT) {

			if (pbody->body->GetLinearVelocity().y >= -maxSpeed)
			{
				float impulse = pbody->body->GetMass() * 1;
				pbody->body->ApplyLinearImpulse({ 0, -impulse }, pbody->body->GetWorldCenter(), true);
			}

		}

		if (app->input->GetKey(SDL_SCANCODE_S) == KEY_REPEAT) {

			if (pbody->body->GetLinearVelocity().y >= -maxSpeed)
			{
				float impulse = pbody->body->GetMass() * 1;
				pbody->body->ApplyLinearImpulse({ 0, impulse }, pbody->body->GetWorldCenter(), true);
			}

		}

		if (app->input->GetKey(SDL_SCANCODE_SPACE) == KEY_DOWN) {

			app->audio->PlayFx(playerJump);
			float impulse = pbody->body->GetMass() * 5;
			pbody->body->ApplyLinearImpulse({ impulse * (float32)SDL_sin(DEGTORAD * angle), 0 }, pbody->body->GetWorldCenter(), true);

			flip = (flip == SDL_FLIP_HORIZONTAL) ? SDL_FLIP_NONE : SDL_FLIP_HORIZONTAL;

		}

	}
	else {

		startTimer = true;
		stateMachineTest->ChangeState("idle");

	}

}

bool Player::SaveState(pugi::xml_node& node) {

	pugi::xml_node playerAttributes = node.append_child("player");
	playerAttributes.append_attribute("x").set_value(this->position.x);
	playerAttributes.append_attribute("y").set_value(this->position.y);
	playerAttributes.append_attribute("angle").set_value(this->angle);
	playerAttributes.append_attribute("lives").set_value(lives);
	playerAttributes.append_attribute("score").set_value(score);

	return true;

}

bool Player::LoadState(pugi::xml_node& node)
{
	pbody->body->SetTransform({ PIXEL_TO_METERS(node.child("player").attribute("x").as_int()), PIXEL_TO_METERS(node.child("player").attribute("y").as_int()) }, node.child("player").attribute("angle").as_int());
	lives = node.child("player").attribute("lives").as_int();
	score = node.child("player").attribute("score").as_int();
	// reset player physics
	pbody->body->SetAwake(false);
	pbody->body->SetAwake(true);

	return true;
}

void Player::CopyParentRotation(PhysBody* parent, PhysBody* child, float xOffset, float yOffset, float angleOffset)
{
	
	float angle = parent->body->GetAngle();

	child->body->SetTransform(
		b2Vec2(
			parent->body->GetTransform().p.x -
			PIXEL_TO_METERS(SDL_cos(angle + DEGTORAD * angleOffset)) * (parent->width + child->width + xOffset),
			parent->body->GetTransform().p.y - 
			PIXEL_TO_METERS(SDL_sin(angle + DEGTORAD * angleOffset)) * (parent->height + child->height + yOffset)),
			DEGTORAD * parent->GetRotation());
}

void Player::moveToSpawnPoint()
{
	position = spawnPosition;

	pbody->body->SetTransform({ PIXEL_TO_METERS(position.x), PIXEL_TO_METERS(position.y) }, 0);

	// reset player physics
	pbody->body->SetAwake(false);
	pbody->body->SetAwake(true);
}

bool Player::CleanUp() {

	app->tex->UnLoad(debugMenuTexture);
	app->tex->UnLoad(texture);

	stateMachineTest->CleanUp();
	delete stateMachineTest;
	stateMachineTest = nullptr;

	app->physics->DestroyBody(pbody);
	app->physics->DestroyBody(groundSensor);
	app->physics->DestroyBody(topSensor);
	app->physics->DestroyBody(leftSensor);
	app->physics->DestroyBody(rightSensor);

	// Theres no need to unload audio fx because they are unloaded when te audio module is cleaned up

	return true;
}

void Player::OnCollision(PhysBody* physA, PhysBody* physB) {

	if (physA->body->GetFixtureList()->IsSensor()) {
		if (physB->ctype == ColliderType::PLATFORM) { //Condicion temporal
			if (physA == groundSensor) {
				LOG("Ground collision");
				isGrounded = true;
				isRotationAllowed = true;
			}
			else if (physA == leftSensor) {
				LOG("Left collision");
				isCollidingLeft = true;
			}
			else if (physA == rightSensor) {
				LOG("Right collision");
				isCollidingRight = true;
			}
		}
		else if(physB->ctype == ColliderType::ENEMY or physB->ctype == ColliderType::BULLET){
			if (physA == groundSensor) {
				LOG("Ground collision");
				isGrounded = true;
			}
		}
	}
	switch (physB->ctype) {

	case ColliderType::ENEMY:
		LOG("Collision ENEMY");
		if (!godMode) {
			if (immunityTimer.ReadSec() >= 1){
				if (lives <= 1)
				{
					//state = EntityState::DEAD;
					stateMachineTest->ChangeState("dead");
					app->map->GetAnimByName("livesAnimation")->currentFrame = 0;
				}
				else{
					// AUDIO DONE player hit
					app->audio->PlayFx(playerHit);
					lives--;
					app->map->GetAnimByName("livesAnimation")->currentFrame++;
					//state = EntityState::HURT;
					stateMachineTest->ChangeState("hurt");
					immunityTimer.Start();
				}
			}
		}
		break;
	case ColliderType::PLATFORM:
		LOG("Collision PLATFORM");
		break;

	case ColliderType::DEATH:
		LOG("Collision DEATH");

		if (!godMode) {
			lives = 0;
			isAlive = false;
			if(stateMachineTest != nullptr)stateMachineTest->ChangeState("dead");
			app->map->GetAnimByName("livesAnimation")->currentFrame = 0;
		}
		break;

	case ColliderType::LIMITS:
		LOG("Collision LIMITS");
		break;
	case ColliderType::WIN:
		//state = EntityState::WIN;
		//stateMachineTest->ChangeState("win");
		app->fade->Fade(app->scene, (Module*)app->finalScene, 60);
		app->entityManager->Disable();
		//app->map->Disable(); deberia de estar activado pero sino crashea :(
		LOG("Collision WIN");
		break;
	case ColliderType::UNKNOWN:
		LOG("Collision UNKNOWN");
		break;

	}
	
}

void Player::EndCollision(PhysBody* physA, PhysBody* physB){

	if (physA->body->GetFixtureList()->IsSensor()) {
		if (physB->ctype == ColliderType::PLATFORM) {
			if (physA == groundSensor) {
				LOG("Ground collision");
				isGrounded = false;
				isRotationAllowed = false;
			}
			else if (physA == leftSensor) {
				LOG("Left collision");
				isCollidingLeft = false;
			}
			else if (physA == rightSensor) {
				LOG("Right collision");
				isCollidingRight = false;
			}
		}
		else if(physB->ctype == ColliderType::ENEMY or physB->ctype == ColliderType::BULLET){
			if (physA == groundSensor) {
				isGrounded = false;
			}
		}
	}
	
}

void Player::OnRaycastHit(b2Fixture* fixture, const b2Vec2& point, const b2Vec2& normal, float32 fraction){
	LOG("Raycast hit");
    std::cout << "Point: " << point.x << ", " << point.y << std::endl;
    std::cout << "Normal: " << normal.x << ", " << normal.y << std::endl;
    std::cout << "Fraction: " << fraction << std::endl;

	//REMOVE
	pointTest = point;
	normalTest = normal;

	if(isRotationAllowed){
		float32 dot = b2Dot(normal, { 0,-1 });
		float32 det = b2Cross(normal, { 0,-1 });
		angle = -b2Atan2(det, dot) * RADTODEG;
	}
}
