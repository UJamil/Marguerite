#include "daisy_field.h"
#include "daisysp.h" // Uncomment this if you want to use the DSP library.

using namespace daisy;
using namespace daisysp;

// Declare a local daisy_field for hardware access
class DaisyField hw;

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
static void AudioCallback(float **in, float **out, size_t size)
{
    hw.ProcessAnalogControls();
    hw.UpdateDigitalControls();

    float Pregain = hw.GetKnobValue(hw.KNOB_1) * 10 + 1.2;
    float Gain = hw.GetKnobValue(hw.KNOB_2) * 100 + 1.2;
    float drywet = hw.GetKnobValue(hw.KNOB_3);

    bypassSoft = hw.GetSwitch(hw.SW_1)->RisingEdge() ? !bypassSoft : bypassSoft;
    bypassHard = hw.GetSwitch(hw.SW_2)->RisingEdge() ? !bypassHard : bypassHard;

    for (size_t i = 0; i < size; i++)
    {

        for (int chn = 0; chn < 2; chn++)
        {
            in[chn][i] *= Pregain;
            float wet = in[chn][i];

            if (!bypassSoft || !bypassHard)
            {
                wet *= Gain;
            }

            if (!bypassSoft)
            {
                wet = softClip(wet);
            }

            if (!bypassHard)
            {
                wet = hardClip(wet);
            }

            out[chn][i] = wet * drywet + in[chn][i] * (1 - drywet);
        }
    }
}

int main(void)
{
    float sample_rate;
    float soft_led = 0;
    float hard_led = 0;
    hw.Init();
    sample_rate = hw.AudioSampleRate();
    bypassHard = bypassSoft = false;

    hw.StartAudio(AudioCallback);
    hw.StartAdc();

    while (1)
    {
        if (bypassSoft)
            soft_led = 255.f;
        else
            soft_led = 0.f;

        if (bypassHard)
            hard_led = 255.f;
        else
            hard_led = 0.f;

        hw.led_driver_.SetLed(DaisyField::LED_KEY_B1, soft_led);
        hw.led_driver_.SetLed(DaisyField::LED_KEY_B2, hard_led);
        hw.led_driver_.SwapBuffersAndTransmit();

        // for (int i = 0; i < 8; i++)
        // {
        // 	petal.SetRingLed((DaisyPetal::RingLed)i, 1.f, 0.f, 0.f);
        // }

        dsy_system_delay(6);
        // Do Stuff InfInitely Here
    }
}
