#include "daisy_petal.h"
#include "daisysp.h" // Uncomment this if you want to use the DSP library.

using namespace daisy;
using namespace daisysp;

// Declare a local daisy_field for hardware access
DaisyField hw;

float hardClip(float in)
{
	in = in > 1.f ? 1.f : in;
	in = in < -1.f ? -1.f : in;
	return in;
}

float softClip(float in)
{
	if (in > 0)
		return 1 - expf(-in);
	return -1 + expf(in);
}

bool bypassHard, bypassSoft;

// This runs at a fixed rate, to prepare audio samples
static void AudioCallback(float *in, float *out, size_t size)
{
    float sine, freq;
    for (size_t i = 0; i < size; i += 2)
    {

        // left out
        out[i] = in[i];

        // right out
        out[i + 1] = in[i + 1];
    }
}

int main(void)
{
    float sample_rate;
    hw.Init();
    sample_rate = hw.AudioSampleRate();

    hw.StartAudio(AudioCallback);
    hw.StartAdc();

    while (1)
    {
        // Do Stuff InfInitely Here
    }
}
