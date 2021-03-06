#pragma config(Sensor, S1,     Touch1,         sensorEV3_Touch)
#pragma config(Sensor, S2,     Touch2,         sensorEV3_Touch)
#pragma config(Sensor, S3,     Colour,         sensorEV3_Color)
#pragma config(Sensor, S4,     sonarSensor,    sensorSONAR)
#pragma config(Motor,  motorB,          leftMotor,     tmotorEV3_Large, PIDControl, driveLeft, encoder)
#pragma config(Motor,  motorC,          rightMotor,    tmotorEV3_Large, PIDControl, driveRight, encoder)
//*!!Code automatically generated by 'ROBOTC' configuration wizard               !!*//

/*
Tile Width: 95mm
Axel: { span: 120mm, circumference: 376.9mm, radius: 60mm)
Wheel { span: 55mm, circumference: 172.7mm, raduis: 27.5mm)
*/

/* The below values are in MM unless otherwise specified. */
const int wheelCircum = 134; // adjusted value for the distance it actually drives.
const int speed = 60; // not in MM. Percentage speed for the motors.
const int turnSpeed = 20; // not in MM. Percentage speed for the motors.
const int tileWidth = 95;
const int laserOffset = 30;

const int towerDistance = 1700;
const int reflectedBlack = 20; // not in MM. Reflection threshold for black.
const int correctionRadius = 125; //distance to drive before expecting a black tile.
const int correctionDistance = 360; //in encoder counts
const int correctionIncrement = 20;

bool missionComplete = false;
bool isBlack = false;
bool objectDetected = false;
bool bumped = false;

int tileCount = 0;

/* Calculates encoder counts required to travel the specified distance.
Takes a parameter in MM to convert. */
float lengthToDegrees(float value) {
	return ((value / wheelCircum) * 360);
}

/* Calculates angle from adjacent and opposite. Converts to degrees. */
float getTurnAngle(float adj) {
	return radiansToDegrees(atan(correctionRadius/adj));
}

/* Function to perform a 90 degree pivot. Takes a direction to turn. */
void pivot(int direction) {
	setMotorSyncEncoder(leftMotor, rightMotor, direction * 100, 180, turnSpeed);
	waitUntilMotorStop(leftMotor);
	waitUntilMotorStop(rightMotor);
}

/* Performs a pivot of a specified angle in a specified direction. */
void pivot(int direction, int angle) {
	setMotorSyncEncoder(leftMotor, rightMotor, direction * 100, angle, turnSpeed);
	waitUntilMotorStop(leftMotor);
	waitUntilMotorStop(rightMotor);
}

/* Drives straight until not detecting black. */
void moveToTileEdge() {
	while(isBlack) {
		setMotorSync(leftMotor, rightMotor, 0, speed);
		displayCenteredBigTextLine(4, "Go To Edge");
	}
}

/* Uses sonar while pivoting and scans for an object. Pivots in increments. */
void scanForObject() {
	pivot(-1);
	int totalPivot = 0;
	while (!objectDetected && totalPivot <= 360) {
		pivot(1, 20);
		totalPivot += 20;
	}
	pivot(1, 30);
}

/* Begins corrective movement left or right. Returns distance it drove in encoder counts. */
int correctiveMove() {
	int adjacent = 0;

	while(!isBlack && adjacent < correctionDistance) {
		setMotorSyncEncoder(leftMotor, rightMotor, 0, correctionIncrement, speed/2);
		displayCenteredBigTextLine(4, "Driven: %d", adjacent);
		adjacent += correctionIncrement;
		waitUntilMotorStop(leftMotor);
		waitUntilMotorStop(rightMotor);
	}

	if (isBlack) {
		setMotorSyncEncoder(leftMotor, rightMotor, 0, 100, speed);
		waitUntilMotorStop(leftMotor);
		if (isBlack) {
			return adjacent;
		}
	}
	return adjacent;
}

/* Begins corrective realign, takes an angle to pivot after moving to center of tile. */
void correctiveRealign(int direction, float angle) {
	setMotorSyncEncoder(leftMotor, rightMotor, 0, lengthToDegrees(tileWidth/3), speed);
	waitUntilMotorStop(leftMotor);
	displayCenteredBigTextLine(4, "Aligning");
	pivot(direction, angle * 2);
}

/* Performs each 45 degree initial check based on direction parameter. */
bool initialCorrect(int dir) {
	pivot(dir, 90);
	if(isBlack) return true;
	pivot(-1 * dir, 90);
	return false;
}

/* If we expect a tile but don't find one; take corrective action.
First check left/right at 45 degree turns. Then check out to each side.*/
bool correction() {

	setMotorSyncEncoder(leftMotor, rightMotor, 0, lengthToDegrees(tileWidth/4), -speed/2);
	waitUntilMotorStop(leftMotor);

	if (initialCorrect(-1)) {
		correctiveRealign(1, 40);
		return true;
	}

	if (initialCorrect(1)) {
		correctiveRealign(-1, 40);
		return true;
	}

	pivot(-1);

	int adjacent = correctiveMove();
	if (adjacent < correctionDistance) {
		if (adjacent == 0) {
			correctiveRealign(1, 90);
			} else {
			correctiveRealign(1, getTurnAngle(adjacent));
		}
		displayCenteredBigTextLine(4, "Adj: %d", adjacent);
		return true;
	}

	pivot(1, 360);
	setMotorSyncEncoder(leftMotor, rightMotor, 0, (correctionDistance + 100 + lengthToDegrees(laserOffset / 2)) / 2, speed);
	waitUntilMotorStop(leftMotor);

	adjacent = correctiveMove();
	if (adjacent < correctionDistance) {
		if (adjacent == 0) {
			correctiveRealign(-1, 90);
			} else {
			correctiveRealign(-1, getTurnAngle(adjacent));
		}
		displayCenteredBigTextLine(4, "Adj: %d", adjacent);
		return true;
	}
	return false;
}

/* Performs a push to push the obstacle off a tile. */
void push() {
	int powerLevel = 50;
	while (powerLevel < 100) {
		setMotorSyncEncoder(leftMotor,rightMotor, 0, lengthToDegrees(100), powerLevel);
		sleep(10);
		powerLevel ++;
	}
	setMotorSyncEncoder(leftMotor,rightMotor, 0, lengthToDegrees(300), powerLevel);
	waitUntilMotorStop(leftMotor);
}

/* Maintains the �??bumped�?? variable, which lets other tasks know if contact is made.*/
task bumpCheck() {
	while (!missionComplete) {
		bumped = SensorValue[Touch1] || SensorValue[Touch2]; // may need updating to work.
	}
}

/* Task which continually runs and maintains the objectDetected variable
 based on whether an object is seen within a certain distance. */
task sonarScan() {
	while (!missionComplete) {
		objectDetected = SensorValue[sonarSensor] < 1000;
		if (objectDetected) {
			if (SensorValue[sonarSensor] < 300) {
				setMotorSyncEncoder(leftMotor, rightMotor, 0, lengthToDegrees(400), speed);
				waitUntilMotorStop(leftMotor);
			}
			displayCenteredBigTextLine(4, "%d", SensorValue[sonarSensor]);
		}
	}
}

/* Task which continually runs and maintains the isBlack
 variable based on what the colour sensor detects. */
task updateColour() {
	while (!missionComplete) {
		isBlack = SensorValue[Colour] <= reflectedBlack;
	}
}

/* Drive forward for a set number of wheel rotations. Then use sonar to search for tower. */
task stageThree() {
	setMotorSyncEncoder(leftMotor, rightMotor, 0, lengthToDegrees(towerDistance), speed);
	waitUntilMotorStop(leftMotor);

	startTask(bumpCheck, 255);
	startTask(sonarScan, 255);
	scanForObject();

	while (!bumped) {
		if (objectDetected) {
				setMotorSync(leftMotor, rightMotor, 0, speed);
			} else {
				scanForObject();
		}
	}
	push();
	missionComplete = true;
}

/* Count 15 black tiles across the floor. Relies on knowing the approximate distance between tiles.
 Because it knows this it can quickly and efficiently realign itself if it runs off course. */
task stageTwo() {
	moveToTileEdge();
	bool newTile = true;

	while (tileCount < 14) {

		if (isBlack && newTile) {
			playTone(800, 30);
			newTile = false;
			tileCount++;
			displayCenteredBigTextLine(4, "%d", tileCount);
			moveToTileEdge();
		}
		displayCenteredBigTextLine(4, "Driving to Radius");
		setMotorSyncEncoder(leftMotor, rightMotor, 0, lengthToDegrees(correctionRadius), speed);
		waitUntilMotorStop(leftMotor);

		if (!isBlack) {
			displayCenteredBigTextLine(4, "Correcting");
			newTile = correction();
			} else {
			newTile = true;
		}
	}
	playTone(800, 30);
	pivot(1);
	startTask(stageThree,255);
}

/* Drive forward to black tile and pivot 90 degrees. */
task stageOne() {

	while (isBlack) {
		setMotorSync(leftMotor, rightMotor, 0, speed);
	}

	while (!isBlack) {
		setMotorSync(leftMotor, rightMotor, 0, speed);
	}

	setMotorSyncEncoder(leftMotor, rightMotor, 0, lengthToDegrees((tileWidth/2) + laserOffset), speed);
	waitUntilMotorStop(leftMotor);
	pivot(1);
	playTone(800, 30);
	tileCount++;
	startTask(stageTwo, 255);
}

/* The main task. Sets up the robot and runs the first tasks. Then waits until mission is complete. */
task main()
{
	isBlack = true;
	setSoundVolume(100);

	startTask(updateColour, 255);

	startTask(stageOne, 255);

	while (!missionComplete) { }

	displayCenteredBigTextLine(4, "Mission Complete!");

	playTone(783, 15);
	sleep(200);
	playTone(1046, 15);
	sleep(200);
	playTone(1318, 15);
	sleep(200);
	playTone(1567, 15);
	sleep(300);
	playTone(1318, 15);
	sleep(200);
	playTone(1567, 30);
	sleep(1000);
}
