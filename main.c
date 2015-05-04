#pragma config(Sensor, S1,     leftColor,      sensorEV3_Color)
#pragma config(Sensor, S2,     sonar,          sensorEV3_Ultrasonic)
#pragma config(Sensor, S4,     rightColor,     sensorEV3_Color)
#pragma config(Motor,  motorB,          leftMotor,     tmotorEV3_Large, PIDControl, driveLeft, encoder)
#pragma config(Motor,  motorC,          rightMotor,    tmotorEV3_Large, PIDControl, driveRight, encoder)
//*!!Code automatically generated by 'ROBOTC' configuration wizard               !!*//

/////////////////////////////////////////////// contstants ////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define BASE_SPEED 20												// the robot's base speed
#define STOPPED_SPEED 0											// the robot's stopped speed
#define MAX_SPEED_INCREASE 10								// the max amount that speed should increase
#define MAX_TURN_TIME 1900									// the max time that the robot can turn
#define HALF_TURN_TIME (MAX_TURN_TIME / 2)	// half of the robot's max turn time
#define MIN_TURN_TIME 400										// the min time that the robot can turn
#define THINKING_TIME 2000									// the time that the robot should sleep
#define THREE_FEET_IN_CM 94									// 3 feet from the front of the robot (in cm)
#define INCH 14															// about an inch from the front of the robot (in cm)
#define CRAWL 10														// DEBUG speed - get rid of later
#define SOUND_VOLUME 10											// the robot's sound volume
#define RATIO_MULTIPLIER 3									// multiplier that helps determine motor speed for line following
/////////////////////////////////////////////// globals ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool isWandering;														// true if the robot is wandering; Otherwise false
bool isLineSearching;												// true if the robot is currently following a line
int dark;																		// detected value for dark colors (black)
int threshold;															// separates dark and light values

/////////////////////////////////////////////// threads ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
task sonarThread();
task wanderThread();
task lineFollowThread();

/////////////////////////////////////////////// prototypes ////////////////////////////////////////////////////////////////////////////////////////////////////////////
void setMotorSpeeds(int leftMotorSpeed, int rightMotorSpeed);
void seeBlack(int leftValue, int rightValue);
void biasedRandomWalk();
void randomPointTurn();
void initLightValues();
bool isWhite(int senseValue);
bool isWithinRange(int distance);
bool isTooClose(int distance);

task main() {
	isLineSearching = false;
	isWandering = true; 
	
	setSoundVolume(SOUND_VOLUME);
	initLightValues();
	startTask(wanderThread);
	startTask(sonarThread); // also starts the line following thread

	while (true){}
}

/* Has the robot follow a line if one appears. */
task lineFollowThread() {
	while (true) {
			int leftValue = SensorValue[leftColor] + 1;
			int rightValue = SensorValue[rightColor] + 1;

			if (seeBlack(leftValue, rightValue)) {
				if (isWandering) {
					isWandering = false;
					stopTask(wanderThread);
				}
				
				float total = (float) leftValue + rightValue;

				float leftRatio = total / rightValue;
				float rightRatio = total / leftValue;

				setMotorSpeed(leftMotor, leftRatio * RATIO_MULTIPLIER + 10);
				setMotorSpeed(rightMotor, rightRatio * RATIO_MULTIPLIER);
			} 
			else if (!isWandering){ // found end of line
				playSound(soundUpwardTones);
				isWandering = true;
				startTask(wanderThread);
			}

		}
}

/* Instantiates reflected light values for the robot. */
void initLightValues() {
		dark = SensorValue[rightColor]; // get dark reading from right sensor
		threshold = dark + 10; // temporary - might need to change
		sleep(3000); // sleep to place robot in proper position
}

/* Returns true if one of the given values represents black. */
bool seeBlack(int leftValue, int rightValue) {
	return leftValue < threshold || rightValue < threshold;
}

/*
 * Turns the robot in either direction using a point turn.
 * Robot will rotate a minimum of MIN_TURN_TIME and a maximum of
 * HALF_TURN_TIME.
 */
void randomPointTurn() {

	if (rand() % 2) { //pivot left
		setMotorSpeeds(-BASE_SPEED, BASE_SPEED);
	} 
	else { //pivot right
		setMotorSpeeds(BASE_SPEED, -BASE_SPEED);
	}

	sleep(random(HALF_TURN_TIME - MIN_TURN_TIME) + MIN_TURN_TIME); //pivots for this long
	setMotorSpeeds(STOPPED_SPEED, STOPPED_SPEED);
}


/*
 * Returns true if an object at the given distance within the robot's distance range
 * but not too close. Otherwise false.
 */
bool isWithinRange(int distance) {
	return distance <= THREE_FEET_IN_CM && INCH < distance;
}

/* Returns true if an object at the given distance is too close to the robot. */
bool isTooClose(int distance) {
	return INCH >= distance;
}

void haltLineThread() {
	isLineSearching = false;
	stopTask(lineFollowThread);
}

void startLineThread() {
	isLineSearching = true;
	startTask(lineFollowThread);
}

task sonarThread() {
	int fDistance;

	while(true) {
		fDistance = (int) getUSDistance(sonar);

		// Object appears within range
		if (isWithinRange(fDistance)) {
			haltLineThread();
			int speed = 89 - (84 - fDistance); // NOTE: make these constants

			setMotorSpeeds(speed, speed);
		}
		// Object is now close enough
		else if (isTooClose(fDistance)) {
				haltLineThread();
				stopAllMotors();

				sleep(THINKING_TIME);
				backward(1,seconds, BASE_SPEED);

				randomPointTurn();
		} 
		// Done with the object, go back to wandering
		else if (!isLineSearching) {
				startLineThread();
		}
	}
}

/*
 * Sets both left and right motor speeds.
 *
 * leftMotorSpeed: the speed of the left motor.
 * rightMotorSpeed: the speed of the right motor. */
void setMotorSpeeds(int leftMotorSpeed, int rightMotorSpeed) {
	setMotorSpeed(leftMotor, leftMotorSpeed);
	setMotorSpeed(rightMotor, rightMotorSpeed);
}

/*
 * Walks forward in a biased fashion. For both the left and the right motor, select a random speed that is between
 * BASE_WALK_SPEED and BASE_WALK_SPEED + MAX_SPEED_INCREASE.
 */
void biasedRandomWalk() {
	setMotorSpeeds(BASE_SPEED + random(MAX_SPEED_INCREASE), BASE_SPEED + random(MAX_SPEED_INCREASE));
}

/* Has the robot move around in a "druken sailor" fashion. */
task wanderThread() {
	int count = 0;

	while (true) {

		if(count > 50) {
			// If the biased random walk hasn't been updated in the last 50 iterations (500 milliseconds)
			biasedRandomWalk();
			count = 0;
		}

		sleep(10);
		count++;
	}
}
