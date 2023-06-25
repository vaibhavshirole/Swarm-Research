#include "Lidar.h"

float increaseScanFrequencyTenth() {
    byte incFreqTenthCmd[2] = {0xA5, 0x09};

    return this->sendFreqCmd(incFreqTenthCmd);
}

float increaseScanFrequencyOne() {
    byte incFreqOneCmd[2] = {0xA5, 0x0B};

    return this->sendFreqCmd(incFreqOneCmd);
}

float decreaseScanFrequencyTenth() {
    byte decFreqTenthCmd[2] = {0xA5, 0x0A};

    return this->sendFreqCmd(decFreqTenthCmd);
}

float decreaseScanFrequencyOne() {
    byte decFreqOneCmd[2] = {0xA5, 0x0C};

    return this->sendFreqCmd(decFreqOneCmd);
}

float getScanFrequency() {
    byte getFreqCommand[2] = {0xA5, 0x0D};

    return sendFreqCmd(getFreqCommand);
}

float sendFreqCmd(byte* commandBytes) {
    byte startCommand[2] = {0xA5, 0x60};
    byte stopCommand[2] = {0xA5, 0x65};
    byte* freqCommand = commandBytes;
    byte frequencyBuf[4];
    byte tempByte;
    //Initialize UART serial port
    //  Serial2.begin(230400, SERIAL_8N1, 16, 17);
    //Turn off lidar
    Serial2.write(stopCommand, 2);
    Serial2.flush();
    delay(1000);
    //Empty UART buffer
    while(Serial2.available()) {
        Serial2.read();
    }
    //Send get frequency command
    Serial2.write(freqCommand, 2);
    //Wait for scan start sign
    while(true) {
        while(!Serial2.available());
        tempByte = Serial2.read();
        while(tempByte != 0xA5) {
            while(!Serial2.available());
            tempByte = Serial2.read();
        }
        while(!Serial2.available());
        tempByte = Serial2.read();
        if(tempByte == 0x5A) break;
    }
    //Ignore bytes (Length, Mode, and Type code)
    for(int i = 0; i < 5; i++) {
        while(!Serial2.available());
        Serial2.read();
    }
    //Read the 4 byte scan frequency value
    Serial2.readBytes(frequencyBuf, 4);
    //Calculate frequency in Hz
    unsigned long frequencyReading = 0;
    for(int i = 0; i < 4; i++) {
        frequencyReading += ((unsigned long)frequencyBuf[i]) << (8 * i);
    }

    Serial2.write(startCommand, 2);
    Serial2.flush();
    delay(1000);

    return ((float)frequencyReading)/100.0;
}

void Lidar::begin() {
//     Serial.println("Starting begin...");
    //Set speed value for speed after reset
    this->currentSpeed = 6.0;
    byte startCommand[2] = {0xA5, 0x60};
    byte restartCommand[2] = {0xA5, 0x40};
    Serial2.begin(230400, SERIAL_8N1, 16, 17);
    delay(1000);
    //Send restart command
    Serial2.write(restartCommand, 2);
    Serial2.flush();
    delay(1000);
    //Empty UART buffer
    while(Serial2.available()) {
        Serial2.read();
    }
    //Send start scan command
    Serial2.write(startCommand, 2);
    Serial2.flush();
    delay(1000);
//     Serial.println("Exiting begin...");
}

byte* Lidar::getDeviceInfo() {
    byte tempByte;
    byte deviceInfoCmd[2] = {0xA5, 0x90};
    byte stopCmd[2] = {0xA5, 0x65};
    byte startCmd[2] = {0xA5, 0x60};
    byte* deviceInfoBuf = (byte*)malloc(sizeof(byte)*8);
    //Turn off Scan
    Serial2.write(stopCmd, 2);
    Serial2.flush();
    //Empty UART Buffer
    while(Serial2.available()) Serial2.read();
    //Get Device info
    Serial2.write(deviceInfoCmd, 2);
    Serial2.flush();
    //Wait for start sign
    while(true) {
        while(!Serial2.available());
        tempByte = Serial2.read();
        while(tempByte != 0xA5) {
            while(!Serial2.available());
            tempByte = Serial2.read();
        }
        while(!Serial2.available());
        tempByte = Serial2.read();
        if(tempByte == 0x5A) break;
    }
    //Skip length, mode, and type code
    for(int i = 0; i < 5; i++) {
        while(!Serial2.available());
        Serial2.read();
    }
    //Get device info bytes
    for(int i = 0; i < 20; i++) {
        while(!Serial2.available());
        deviceInfoBuf[i] = Serial2.read();
    }
    //Turn on Scan
    Serial2.write(startCmd, 2);
    Serial2.flush();

    return deviceInfoBuf;
}

void Lidar::freePointCloud(pointCloud* inPointCloud) {
    free(inPointCloud->distances);
    free(inPointCloud->intensities);
    free(inPointCloud->angles);
    free(inPointCloud->pointFlags);
    free(inPointCloud->pointTimes);
}

pointCloud Lidar::getPoints(unsigned int pointsRequested) {
    //Does not turn on or off the Lidar
    unsigned int pointsCollected = 0;
    byte sampleQuantity;
    byte tempByte;
    unsigned int startAngle;
    unsigned int endAngle;
    byte startAngleBuf[2];
    byte endAngleBuf[2];
    //Allocate memory for sample data
    byte** byteTriples = (byte**)malloc(pointsRequested * sizeof(byte*));
    for(int i = 0; i < pointsRequested; i++) {
        byteTriples[i] = (byte*)malloc(3*sizeof(byte));
    }
    //Allocate memory for point array using polar coordinates
    unsigned int* calculatedDistances = (unsigned int*)malloc(pointsRequested * sizeof(unsigned int));
    float* calculatedAngles = (float*)malloc(pointsRequested * sizeof(float));
    byte* tempIntensities = (byte*)malloc(pointsRequested * sizeof(byte));
    byte* tempPointFlags = (byte*)malloc(pointsRequested * sizeof(byte));
    unsigned long* tempPointTimeStamps = (unsigned long*)malloc(pointsRequested * sizeof(unsigned long));

    //Empty UART buffer
    while(Serial2.available()) {
        Serial2.read();
    }
    while(pointsCollected < pointsRequested) {
        //Wait for packet header
        while(true) {
            while(!Serial2.available());
            tempByte = Serial2.read();
            while(tempByte != 0xAA) {
                while(!Serial2.available());
                tempByte = Serial2.read();
            }
            while(!Serial2.available());
            tempByte = Serial2.read();
            if(tempByte == 0x55) break;
        }
        //Package type (CT bits)
        while(!Serial2.available());
        //    CTbits = Serial2.read();
        Serial2.read();
        //Get sample quantity
        while(!Serial2.available());
        sampleQuantity = Serial2.read();
        //Get Start angle
        Serial2.readBytes(startAngleBuf, 2);
        //Get End angle
        Serial2.readBytes(endAngleBuf, 2);
        //Skip Check code
        while(!Serial2.available());
        tempByte = Serial2.read();
        while(!Serial2.available());
        tempByte = Serial2.read();
        //Collect points
        for(int i = 0; i < sampleQuantity; i++) {
            while(!Serial2.available());
            byteTriples[pointsCollected][0] = Serial2.read();
            while(!Serial2.available());
            byteTriples[pointsCollected][1] = Serial2.read();
            while(!Serial2.available());
            byteTriples[pointsCollected][2] = Serial2.read();
            tempPointTimeStamps[pointsCollected] = micros();
            pointsCollected++;
            if(pointsCollected >= pointsRequested) break;
        }
        //Calculate point angles
        startAngle = (unsigned int)startAngleBuf[0];
        startAngle += ((unsigned int)startAngleBuf[1]) << 8;
        endAngle = (unsigned int)endAngleBuf[0];
        endAngle += ((unsigned int)endAngleBuf[1]) << 8;

        float angleFSA = ((float)(startAngle >> 1))/64;
        //Manage case of single measured point
        if(sampleQuantity == 1) {
            calculatedAngles[pointsCollected - sampleQuantity] = angleFSA;
            continue;
        }
        float angleLSA = ((float)(endAngle >> 1))/64;
        //Calculate angle difference
        float angleDiff;
        if(angleLSA - angleFSA < 0) angleDiff = angleLSA - angleFSA + 360;
        else angleDiff = angleLSA - angleFSA;
        for(int i = pointsCollected - sampleQuantity; i < pointsCollected; i++) {
            calculatedAngles[i] = angleDiff / ((float)sampleQuantity - 1) * ((float)i - (pointsCollected - sampleQuantity)) + angleFSA;
            if(calculatedAngles[i] >= 360) calculatedAngles[i] -= 360;
        }
    }
    //Calculate distance, intensity, and flag values from byteTriple
    for(int i = 0; i < pointsRequested; i++) {
        calculatedDistances[i] = ((unsigned int)byteTriples[i][2]) << 6;
        calculatedDistances[i] += ((unsigned int)byteTriples[i][1]) >> 2;
        tempIntensities[i] = byteTriples[i][0];
        tempPointFlags[i] = byteTriples[i][0] & 0b00000011;
        free(byteTriples[i]);
    }
    free(byteTriples);
    //Package all values into struct
    pointCloud myCloud;
    myCloud.numPoints = pointsCollected;
    myCloud.distances = calculatedDistances;
    myCloud.angles = calculatedAngles;
    myCloud.intensities = tempIntensities;
    myCloud.pointFlags = tempPointFlags;
    myCloud.pointTimes = tempPointTimeStamps;

    return myCloud;
}

void Lidar::printPointCloud(pointCloud tempPC) {
    unsigned int totalPoints = tempPC.numPoints;
    Serial.print("pointTimes");
    Serial.print(",");
    Serial.print("angles");
    Serial.print(",");
    Serial.print("distances");
    Serial.print(",");
    Serial.print("intensities");
    Serial.print(",");
    Serial.println("pointFlags");
    for(int i = 0; i < totalPoints; i++) {
        Serial.print(tempPC.pointTimes[i]);
        Serial.print(",");
        Serial.print(tempPC.angles[i]);
        Serial.print(",");
        Serial.print(tempPC.distances[i]);
        Serial.print(",");
        Serial.print(tempPC.intensities[i]);
        Serial.print(",");
        Serial.println(tempPC.pointFlags[i]);
    }
}

float setSpeed(float inputSpeed) {
    int difference = (int)(inputSpeed - this->currentSpeed);
    if(difference < 0) {
        for(int i = 0; i > difference; i--) {
            float tempSpeed = this->decreaseScanFrequencyOne();
            if(tempSpeed == this->currentSpeed) break;
            this->currentSpeed = tempSpeed;
        }
        while(this->currentSpeed > inputSpeed) {
            float tempSpeed = this->decreaseScanFrequencyTenth();
            if(tempSpeed == this->currentSpeed) break;
            this->currentSpeed = tempSpeed;
        }
    }
    else if(difference > 0) {
        for(int i = 0; i < difference; i++) {
            float tempSpeed = this->increaseScanFrequencyOne();
            if(tempSpeed == this->currentSpeed) break;
            this->currentSpeed = tempSpeed;
        }
        while(this->currentSpeed < inputSpeed) {
            float tempSpeed = this->increaseScanFrequencyTenth();
            if(tempSpeed == this->currentSpeed) break;
            this->currentSpeed = tempSpeed;
        }
    }

    return this->currentSpeed;
}

float getResolution() {
    //Return upper bound of points collected per degree of rotation
    return 12786.0/(360 * this->currentSpeed);
}

pointRings Lidar::sweep(float numRotations) {
//     Serial.println("Starting sweep...");
    //Error check
    if(numRotations <= 0) {
        pointRings tempPR;
        tempPR.numRings = 0;
        return tempPR;
    }
    //Determine upper bound on memory allocation
    int numPoints = (int) (this->getResolution() * numRotations * 360);
    float sweepStartAngle = -1;
    float sweepEndAngle = -1;
    unsigned int numFullRotations = (int)numRotations;
    unsigned int curNumRotations = 0;

    //Does not turn on or off the Lidar
    unsigned int pointsCollected = 0;
    byte sampleQuantity;
    byte tempByte;
    unsigned int startAngle;
    unsigned int endAngle;
    byte startAngleBuf[2];
    byte endAngleBuf[2];
    //Allocate memory for sample data
    byte** byteTriples = (byte**)malloc(numPoints * sizeof(byte*));
    for(int i = 0; i < numPoints; i++) {
        byteTriples[i] = (byte*)malloc(3*sizeof(byte));
    }
    //Allocate memory for point array using polar coordinates
    unsigned int* calculatedDistances = (unsigned int*)malloc(numPoints * sizeof(unsigned int));
    float* calculatedAngles = (float*)malloc(numPoints * sizeof(float));
    byte* tempIntensities = (byte*)malloc(numPoints * sizeof(byte));
    byte* tempPointFlags = (byte*)malloc(numPoints * sizeof(byte));
    unsigned long* tempPointTimeStamps = (unsigned long*)malloc(numPoints * sizeof(unsigned long));

    //Empty UART buffer
    while(Serial2.available()) {
        Serial2.read();
    }
//     Serial.println("Starting to collect points");
    while(pointsCollected < numPoints) {
        //Wait for packet header
        while(true) {
            while(!Serial2.available());
            tempByte = Serial2.read();
            while(tempByte != 0xAA) {
                while(!Serial2.available());
                tempByte = Serial2.read();
            }
            while(!Serial2.available());
            tempByte = Serial2.read();
            if(tempByte == 0x55) break;
        }
        //Package type (CT bits)
        while(!Serial2.available());
        //    CTbits = Serial2.read();
        Serial2.read();
        //Get sample quantity
        while(!Serial2.available());
        sampleQuantity = Serial2.read();
        //Get Start angle
        Serial2.readBytes(startAngleBuf, 2);
        //Get End angle
        Serial2.readBytes(endAngleBuf, 2);
        //Skip Check code
        while(!Serial2.available());
        tempByte = Serial2.read();
        while(!Serial2.available());
        tempByte = Serial2.read();
        //Collect points
//         Serial.println("Collecting points");
        for(int i = 0; i < sampleQuantity; i++) {
            while(!Serial2.available());
            byteTriples[pointsCollected][0] = Serial2.read();
            while(!Serial2.available());
            byteTriples[pointsCollected][1] = Serial2.read();
            while(!Serial2.available());
            byteTriples[pointsCollected][2] = Serial2.read();
            tempPointTimeStamps[pointsCollected] = micros();
            pointsCollected++;
            if(pointsCollected >= numPoints) break;
        }
        //Calculate point angles
        startAngle = (unsigned int)startAngleBuf[0];
        startAngle += ((unsigned int)startAngleBuf[1]) << 8;
        endAngle = (unsigned int)endAngleBuf[0];
        endAngle += ((unsigned int)endAngleBuf[1]) << 8;

        float angleFSA = ((float)(startAngle >> 1))/64;
        //Set sweepStartAngle and sweepEndAngle if applicable
        if(sweepStartAngle == -1) {
            sweepStartAngle = angleFSA;
            sweepEndAngle = 360 * (numRotations - ((int)numRotations)) + angleFSA;
            if(sweepEndAngle >= 360) sweepEndAngle -= 360;
        }
        //Manage case of single measured point
        if(sampleQuantity == 1) {
            calculatedAngles[pointsCollected - sampleQuantity] = angleFSA;
            continue;
        }
        float angleLSA = ((float)(endAngle >> 1))/64;
        //Calculate angle difference
        float angleDiff;
        if(angleLSA - angleFSA < 0) angleDiff = angleLSA - angleFSA + 360;
        else angleDiff = angleLSA - angleFSA;
        for(int i = pointsCollected - sampleQuantity; i < pointsCollected; i++) {
            calculatedAngles[i] = angleDiff / ((float)sampleQuantity - 1) * ((float)i - (pointsCollected - sampleQuantity)) + angleFSA;
            if(calculatedAngles[i] >= 360) calculatedAngles[i] -= 360;
        }
        static bool fullRotCounted = true;
        static bool quitSweep = false;
        //Determine whether number of requested rotations is met
        for(int i = pointsCollected - sampleQuantity; i < pointsCollected; i++) {
            //Should fullRotCounted flag be reset?
            if(fullRotCounted && calculatedAngles[i] < sweepStartAngle) fullRotCounted = false;
            //Determine whether number of requested rotations is met
            if(curNumRotations >= numFullRotations) {
                //sweepEndAngle after sweepStartAngle
                if(sweepEndAngle >= sweepStartAngle && calculatedAngles[i] >= sweepEndAngle) {
                    quitSweep = true;
                    break;
                }
                //sweepEndAngle before sweepStartAngle
                else if(sweepEndAngle < sweepStartAngle && !fullRotCounted && calculatedAngles[i] >= sweepEndAngle) {
                    quitSweep = true;
                    break;
                }
            }
            if(fullRotCounted) continue;
            if(i != 0 && calculatedAngles[i] >= sweepStartAngle) {
                curNumRotations++;
                fullRotCounted = true;
            }
        }
        if(quitSweep) break;
//         Serial.println("Going to top of while loop");
    }
    //Calculate distance, intensity, and flag values from byteTriple
    for(int i = 0; i < pointsCollected; i++) {
        calculatedDistances[i] = ((unsigned int)byteTriples[i][2]) << 6;
        calculatedDistances[i] += ((unsigned int)byteTriples[i][1]) >> 2;
        tempIntensities[i] = byteTriples[i][0];
        tempPointFlags[i] = byteTriples[i][0] & 0b00000011;
        free(byteTriples[i]);
    }
    free(byteTriples);
    //Package all values into struct
    pointCloud myCloud;
    myCloud.numPoints = pointsCollected;
    myCloud.distances = calculatedDistances;
    myCloud.angles = calculatedAngles;
    myCloud.intensities = tempIntensities;
    myCloud.pointFlags = tempPointFlags;
    myCloud.pointTimes = tempPointTimeStamps;

    pointRings tempRings = Lidar::cloudToRings(myCloud, (int)numRotations);
    freePointCloud(&myCloud);

//     Serial.println("Exiting sweep...");

    return tempRings;
}

pointRings Lidar::cloudToRings(pointCloud inCloud, int numRotations) {
//     Serial.println("Entering cloudToRings...");
    pointRings tempRings;
    tempRings.numRings = numRotations;
    tempRings.ringArray = (pointCloud*)malloc(numRotations * sizeof(pointCloud));
    unsigned int startIndex = 0;
    unsigned int curRing = 0;
    float startAngle = inCloud.angles[0];
    for(int i = 1; i < inCloud.numPoints; i++) {
        // Typical detection of ring completion
        if(inCloud.angles[i] >= startAngle && inCloud.angles[i - 1] < startAngle) {
            // End ring and start next ring
            pointCloud tempCloud;
            // Acount for repeated point in the end of the ring
            tempCloud.numPoints = i - startIndex + 1;
            tempCloud.distances = (unsigned int*)malloc(tempCloud.numPoints * sizeof(unsigned int));
            tempCloud.intensities = (byte*)malloc(tempCloud.numPoints * sizeof(byte));
            tempCloud.angles = (float*)malloc(tempCloud.numPoints * sizeof(float));
            tempCloud.pointFlags = (byte*)malloc(tempCloud.numPoints * sizeof(byte));
            tempCloud.pointTimes = (unsigned long*)malloc(tempCloud.numPoints * sizeof(unsigned long));
            for(int j = startIndex; j < i; j++) {
                // Copy inCloud data into tempCloud
                tempCloud.distances[j - startIndex] = inCloud.distances[j];
                tempCloud.intensities[j - startIndex] = inCloud.intensities[j];
                tempCloud.angles[j - startIndex] = inCloud.angles[j];
                tempCloud.pointFlags[j - startIndex] = inCloud.pointFlags[j];
                tempCloud.pointTimes[j - startIndex] = inCloud.pointTimes[j];
            }
            // Complete ring with copied final point
            tempCloud.distances[tempCloud.numPoints - 1] = tempCloud.distances[0];
            tempCloud.intensities[tempCloud.numPoints - 1] = tempCloud.intensities[0];
            tempCloud.angles[tempCloud.numPoints - 1] = tempCloud.angles[0];
            tempCloud.pointFlags[tempCloud.numPoints - 1] = tempCloud.pointFlags[0];
            tempCloud.pointTimes[tempCloud.numPoints - 1] = tempCloud.pointTimes[0];
            // Save cloud to ring
            tempRings.ringArray[curRing] = tempCloud;
            curRing++;
            // Set up start index for next ring
            startIndex = i;
            startAngle = inCloud.angles[startIndex];
        }
        // If start angle is large, but less than 360 degrees
        else if(inCloud.angles[i] < startAngle && inCloud.angles[i - 1] < startAngle && inCloud.angles[i] < inCloud.angles[i - 1]) {
            // End ring and start next ring
            pointCloud tempCloud;
            // Acount for repeated point in the end of the ring
            tempCloud.numPoints = i - startIndex + 1;
            tempCloud.distances = (unsigned int*)malloc(tempCloud.numPoints * sizeof(unsigned int));
            tempCloud.intensities = (byte*)malloc(tempCloud.numPoints * sizeof(byte));
            tempCloud.angles = (float*)malloc(tempCloud.numPoints * sizeof(float));
            tempCloud.pointFlags = (byte*)malloc(tempCloud.numPoints * sizeof(byte));
            tempCloud.pointTimes = (unsigned long*)malloc(tempCloud.numPoints * sizeof(unsigned long));
            for(int j = startIndex; j < i; j++) {
                // Copy inCloud data into tempCloud
                tempCloud.distances[j - startIndex] = inCloud.distances[j];
                tempCloud.intensities[j - startIndex] = inCloud.intensities[j];
                tempCloud.angles[j - startIndex] = inCloud.angles[j];
                tempCloud.pointFlags[j - startIndex] = inCloud.pointFlags[j];
                tempCloud.pointTimes[j - startIndex] = inCloud.pointTimes[j];
            }
            // Complete ring with copied final point
            tempCloud.distances[tempCloud.numPoints - 1] = tempCloud.distances[0];
            tempCloud.intensities[tempCloud.numPoints - 1] = tempCloud.intensities[0];
            tempCloud.angles[tempCloud.numPoints - 1] = tempCloud.angles[0];
            tempCloud.pointFlags[tempCloud.numPoints - 1] = tempCloud.pointFlags[0];
            tempCloud.pointTimes[tempCloud.numPoints - 1] = tempCloud.pointTimes[0];
            // Save cloud to ring
            tempRings.ringArray[curRing] = tempCloud;
            curRing++;
            // Set up start index for next ring
            startIndex = i;
            startAngle = inCloud.angles[startIndex];
            //Decrement i so that current point will be used for next ring
            i--;
        }
        if(curRing >= numRotations) return tempRings;
    }
//     Serial.println("Exiting cloudToRings...");

    return tempRings;
}

std::vector<polarPoint> Lidar::lerpDist(std::vector<float> inAngles, pointRings inRings) {
    inAngles = Lidar::proccessAngleVector(inAngles);
    int inAnglesSize = inAngles.size();
    std::vector<float> outDists;
    outDists.assign(inAnglesSize, 0);

    for(int j = 0; j < inRings.numRings; j++) {
        pointCloud inCloud = inRings.ringArray[j];
        //Determine what angle will be interpolated first
        int angleIndex = -1;
        for(int i = 0; i < inAnglesSize; i++) {
            // Find previous angle index
            int prevAngleIndex = -1;
            if(i == 0) prevAngleIndex = inAnglesSize - 1;
            else prevAngleIndex = i - 1;

            // Find which input angle should be interpolated first
            if(inAngles.at(i) > inCloud.angles[0] && inAngles.at(prevAngleIndex) < inCloud.angles[0]) {
                angleIndex = i;
                break;
            }
            else if(inAngles.at(i) > inCloud.angles[0] && inAngles.at(prevAngleIndex) > inCloud.angles[0] && inAngles.at(prevAngleIndex) > inAngles.at(i)) {
                angleIndex = i;
                break;
            }
            else if(inAngles.at(i) < inCloud.angles[0] && inAngles.at(prevAngleIndex) < inCloud.angles[0] && inAngles.at(prevAngleIndex) > inAngles.at(i)) {
                angleIndex = i;
                break;
            }
        }

        for(int i = 1; i < inCloud.numPoints; i++) {
            if(inAngles.at(angleIndex) > inCloud.angles[i - 1] && inAngles.at(angleIndex) < inCloud.angles[i]) {
                //Angle difference between measured points surrounding angle of interest
                double angleSpan = fmod((360 - (inCloud.angles[i - 1] - inCloud.angles[i])), 360);
                //Angle difference between requested angle and previous point angle
                double angleDiff = fmod((360 - (inCloud.angles[i - 1] - inAngles.at(angleIndex))), 360);
                float tValue = (float)(angleDiff / angleSpan);
                //a+t*(b−a) linear interpolation
                // Casting from unsigned int to int may cause strange behaviours if the unsigned int is greater than the max int size
                outDists.at(angleIndex) += inCloud.distances[i - 1] + tValue * (((int)inCloud.distances[i]) - ((int)inCloud.distances[i - 1]));
                angleIndex++;
                if(angleIndex >= inAnglesSize) angleIndex = angleIndex - inAnglesSize;
            }
            //If inAngles.at(angleIndex) is near 360, but less than 360
            else if(inAngles.at(angleIndex) > inCloud.angles[i] && inAngles.at(angleIndex) > inCloud.angles[i - 1] && inCloud.angles[i - 1] > inCloud.angles[i]) {
                //Angle difference between measured points surrounding angle of interest
                double angleSpan = fmod((360 - (inCloud.angles[i - 1] - inCloud.angles[i])), 360);
                //Angle difference between requested angle and previous point angle
                double angleDiff = fmod((360 - (inCloud.angles[i - 1] - inAngles.at(angleIndex))), 360);
                float tValue = (float)(angleDiff / angleSpan);
                //a+t*(b−a) linear interpolation
                // Casting from unsigned int to int may cause strange behaviours if the unsigned int is greater than the max int size
                outDists.at(angleIndex) += inCloud.distances[i - 1] + tValue * (((int)inCloud.distances[i]) - ((int)inCloud.distances[i - 1]));
                angleIndex++;
                if(angleIndex >= inAnglesSize) angleIndex = angleIndex - inAnglesSize;
            }
            // If angle is near 0, but greater than 0
            else if(inAngles.at(angleIndex) < inCloud.angles[i] && inAngles.at(angleIndex) < inCloud.angles[i - 1] && inCloud.angles[i] < inCloud.angles[i - 1]) {
                //Angle difference between measured points surrounding angle of interest
                double angleSpan = fmod((360 - (inCloud.angles[i - 1] - inCloud.angles[i])), 360);
                //Angle difference between requested angle and previous point angle
                double angleDiff = fmod((360 - (inCloud.angles[i - 1] - inAngles.at(angleIndex))), 360);
                float tValue = (float)(angleDiff / angleSpan);
                //a+t*(b−a) linear interpolation
                // Casting from unsigned int to int may cause strange behaviours if the unsigned int is greater than the max int size
                outDists.at(angleIndex) += inCloud.distances[i - 1] + tValue * (((int)inCloud.distances[i]) - ((int)inCloud.distances[i - 1]));
                angleIndex++;
                if(angleIndex >= inAnglesSize) angleIndex = angleIndex - inAnglesSize;
            }
            // If exact angle match
            else if(inAngles.at(angleIndex) == inCloud.angles[i]) {
                outDists.at(angleIndex) += inCloud.distances[i];
                angleIndex++;
                if(angleIndex >= inAnglesSize) angleIndex = angleIndex - inAnglesSize;
            }
        }
    }
    // Divide distance sum by numRings to find average value
    for(int i = 0; i < outDists.size(); i++) outDists.at(i) /= inRings.numRings;
    // Package sorted angles vector and distance vector into struct
    std::vector<polarPoint> retPoints;
    for(int i = 0; i < inAnglesSize; i++) {
        polarPoint tempPoint;
        tempPoint.angle = inAngles.at(i);
        tempPoint.distance = outDists.at(i);
        retPoints.push_back(tempPoint);
    }

    return retPoints;
}

std::vector<float> Lidar::proccessAngleVector(std::vector<float> inAngles) {
    // Ensure that all elements of inAngles are within [0, 360)
    // Then sort elements of inAngles
    int inAnglesSize = inAngles.size();
    for(int i = 0; i < inAnglesSize; i++) {
        if(inAngles.at(i) < 0 || inAngles.at(i) >= 360) {
            inAngles.at(i) = inAngles.at(i) - std::floor(inAngles.at(i) / 360.0) * 360;
        }
    }
    std::sort(inAngles.begin(), inAngles.end());

    return inAngles;
}
