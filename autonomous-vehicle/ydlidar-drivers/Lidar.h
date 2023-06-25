#ifndef Lidar_h
#define Lidar_h

#include <math.h>
#include <vector.h>

typedef float angle_t;
typedef float distance_t;

typedef struct {
        float increaseScanFrequencyTenth();
        float increaseScanFrequencyOne();
        float decreaseScanFrequencyTenth();
        float decreaseScanFrequencyOne();
        float getScanFrequency();
        void begin();
        byte* getDeviceInfo();
        void freePointCloud(pointCloud* inPointCloud);
        pointCloud getPoints(unsigned int pointsRequested);
        void printPointCloud(pointCloud tempPC);
        float setSpeed(float inputSpeed);
        pointRings sweep(float numRotations);
        std::vector<polarPoint> lerpDist(std::vector<float> inAngles, pointRings inRings);
        float sendFreqCmd(byte* commandBytes);
        float getResolution();
        float currentSpeed;
        std::vector<float> proccessAngleVector(std::vector<float>);
        pointRings cloudToRings(pointCloud inCloud, int numRotations);
} Lidar;

struct pointCloud {
    unsigned int numPoints;
    unsigned int* distances;
    byte* intensities;
    float* angles;
    byte* pointFlags;
    unsigned long* pointTimes;
};

struct pointRings {
    unsigned int numRings;
    pointCloud* ringArray;
};

struct polarPoint {
    angle_t angle;
    distance_t distance;
};

#endif