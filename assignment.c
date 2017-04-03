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
encoderCount = 360
*/
const int wheelCircum = 134;
const int speed = 50;
const int turnSpeed = 20;
const int tileWidth = 95;
const int laserOffset = 30;

const int towerDistance = 1700;
const int reflectedBlack = 20;
const int correctionRadius = 110;
const int correctionDistance = 360; //in encoder counts
const int correctionIncrement = 20;

bool missionComplete = false;
bool isBlack = false;
bool objectDetected = false;
bool bumped = false;

int tileCount = 0;

/* Calculates encoder counts required to travel the specified distance. */
float lengthToDegrees(float value) {
	return ((value / wheelCircum) * 360);
}

/* Calculates angle from adjacent and opposite. */
float getTurnAngle(float adj) {
	return radiansToDegrees(atan(correctionRadius/adj));
}

/* Plays lost tone sequence. */
void lost() {
	playTone(780, 50);
	sleep(200);
	playTone(500, 60);
	sleep(200);
	playTone(300, 100);
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

/* Uses sonar to pivot and scan for an object. */
void scanForObject() {
	pivot(-1);
	int totalPivot = 0;
	while (!objectDetected && totalPivot <= 360) {
		pivot(1, 30);
		totalPivot += 30;
	}
	pivot(1, 50);
}

/* Begins corrective movement left or right. Returns distance it drove in encoder counts. */
int correctiveMove() {
	playTone(500, 20);
	sleep(20);
	playTone(1000, 20);
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
	playTone(1000, 20);
	sleep(20);
	playTone(500, 20);
	if (isBlack) {
		setMotorSyncEncoder(leftMotor, rightMotor, 0, lengthToDegrees(tileWidth/3), speed);
		waitUntilMotorStop(leftMotor);
		displayCenteredBigTextLine(4, "Aligning");
		pivot(direction, angle * 2);
	}
}

bool initialCorrect(int dir) {
	pivot(dir, 90);
	if(isBlack) return true;
	pivot(-1 * dir, 90);
	return false;
}

/* If we expect a tile but don't find one; take corrective action. */
bool correction() {

	if (initialCorrect(-1)) {
		setMotorSyncEncoder(leftMotor, rightMotor, 0, lengthToDegrees(tileWidth/3), speed/2);
		waitUntilMotorStop(leftMotor);
		pivot(1, 80);
		return true;
	}

	if (initialCorrect(1)) {
		setMotorSyncEncoder(leftMotor, rightMotor, 0, lengthToDegrees(tileWidth/3), speed/2);
		waitUntilMotorStop(leftMotor);
		pivot(-1, 80);
		return true;
	}

	pivot(-1);

	//setMotorSyncEncoder(leftMotor, rightMotor, 0, lengthToDegrees(laserOffset / 2), -1 * speed);
	//waitUntilMotorStop(leftMotor);
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

	lost();
	return false;
}

/* Performs a push to push the obstable off a tile. */
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

task bumpCheck() {
	while (!missionComplete) {
		bumped = SensorValue[Touch1] || SensorValue[Touch2]; // may need updating to work.
	}
}

task sonarScan() {
	while (!missionComplete) {
		objectDetected = SensorValue[sonarSensor] < 1000;
		if (objectDetected) {
			displayCenteredBigTextLine(4, "Detected");
			} else {
			displayCenteredBigTextLine(4, "");
		}
	}
}

task updateColour() {
	while (!missionComplete) {
		isBlack = SensorValue[Colour] <= reflectedBlack;
	}
}

/* Drive forward for a set number of wheel rotations. Then look for tower*/
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



/* Count 15 black tiles across the floor */
task stageTwo() {
	moveToTileEdge();
	bool newTile = true;

	while (tileCount < 15) {

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

/* Drive forward to black tile and pivot */
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
