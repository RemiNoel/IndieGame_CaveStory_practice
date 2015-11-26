#ifndef PLAYER_H
#define PLAYER_H

#include "Globals.h"
#include "AnimatedSprite.h"
#include "Slope.h"

/*	Player class
*	Holds the logic for Player Quote
*/

class Player : public AnimatedSprite {

public:
	Player();
	Player(Graphics &graphics, Vector2 spawnPoint);
	void draw(Graphics &graphics);
	void update(float elapsedTime);

	/*	void moveRight
	*	Moves the player right by dx
	*/
	void moveRight();

	/*	void moveLeft
	*	Moves the player left by -dx
	*/
	void moveLeft();

	/*	void stopMoving
	*	Stops moving the player
	*/
	void stopMoving();

	/*	void lookUp
	*	The player look up
	*/
	void lookUp();

	/*	void stopLookingUp
	*	the player stops looking up
	*/
	void stopLookingUp();

	/*	void lookDown
	*	The player look down OR interacts(looks around)
	*/
	void lookDown();

	/*	void stopLookingDown
	*	the player stops looking down
	*/
	void stopLookingDown();

	/*	void jump
	*	Makes the character(Quote) jump
	*/
	void jump();

	virtual void animationDone(std::string currentAnimation);
	virtual void setupAnimations();

	/*	void handleTileCollisions
	*	Handles collisions with ALL tiles the player is colliding with
	*/
	void handleTileCollisions(std::vector<Rectangle> &others);

	void handleSlopeCollisions(std::vector<Slope> &others);

	const float getX() const;
	const float getY() const;

private:
	float _dx, _dy;

	Direction _facing;

	bool _grounded;

	bool _lookingUp;
	bool _lookingDown;
};

#endif