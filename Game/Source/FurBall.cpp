#include "FurBall.h"
#include "App.h"
#include "Textures.h"
#include "Audio.h"
#include "Input.h"
#include "Render.h"
#include "Scene.h"
#include "Log.h"
#include "Point.h"
#include "Physics.h"

#ifdef __linux__
#include <Box2D/Dynamics/b2Body.h>
#endif

FurBall::FurBall() : Entity(EntityType::FURBALL)
{
	name.Create("FurBall");
}

FurBall::~FurBall() {}

bool FurBall::Awake() {

	position = {0,0};
	texturePath = "Assets/Textures/cat-fur-ball.png";

	return true;
}

bool FurBall::Start() {

	//initilize textures
	texture = app->tex->Load(texturePath);
	pbody = app->physics->CreateCircle(position.x, position.y, 2, bodyType::DYNAMIC);
	pbody->ctype = ColliderType::BULLET;
	pbody->listener = this;

	return true;
}

bool FurBall::Update(float dt)
{
	// L07 DONE 4: Add a physics to an FurBall - update the position of the object from the physics.  
	position.x = METERS_TO_PIXELS(pbody->body->GetTransform().p.x);
	position.y = METERS_TO_PIXELS(pbody->body->GetTransform().p.y);

	app->render->DrawTexture(texture, position.x - 2, position.y - 2, NULL, 1.0f,  pbody->body->GetAngle()*RADTODEG);

	return true;
}

bool FurBall::CleanUp()
{

	app->tex->UnLoad(texture);
	return true;
}

void FurBall::OnCollision(PhysBody* physA, PhysBody* physB)
{
	switch (physB->ctype) {

	case ColliderType::ENEMY:
		pbody->ctype = ColliderType::UNKNOWN;
		break;
	}
}
