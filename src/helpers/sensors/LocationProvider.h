#pragma once

#include "Mesh.h"


class LocationProvider {

public:
    virtual long getLatitude() = 0;
    virtual long getLongitude() = 0;
    virtual bool isValid() = 0;
    virtual long getTimestamp() = 0;
    virtual int getNumSatellites() = 0;
    virtual long getAltitude() = 0;  // in millimeters
    virtual void reset();
    virtual void begin();
    virtual void stop();
    virtual void loop();
};
