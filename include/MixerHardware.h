#pragma once
#include "sys.h"

class MixerHardware {
public:
	MixerHardware();
	virtual ~MixerHardware();
	virtual void flush() = 0;
	virtual void update(int *pAudioData, int count) = 0;
	virtual void update(short *pAudioData, int count) = 0;
	virtual u64 samplepos() = 0;
	virtual void start() = 0;
	virtual void stop() = 0;
};

inline MixerHardware::MixerHardware() {

}

inline MixerHardware::~MixerHardware() {
}
