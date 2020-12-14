#include "daisy_field.h"
#include "daisysp.h" // Uncomment this if you want to use the DSP library.
#include <stdio.h>
#include <string>

using namespace daisy;
using namespace daisysp;

// Declarations
class DaisyField hw;

uint8_t knobs[8];
char strbuff[128];

struct ConditionalUpdate
{
    float oldVal = 0;
    bool Process(float newVal)
    {
        if (abs(newVal - oldVal) > 0.04f)
        {
            oldVal = newVal;
            return true;
        }
        return false;
    }
};

ConditionalUpdate condUpdate;

struct Filter
{
    Svf filt;
    float amp_, drv_, freq_, res_, rate_;
    int type_;

    void Init(float samplerate)
    {
        filt.Init(samplerate);
        filt.SetRes(0.5f);
        filt.SetDrive(0.002f);
        filt.SetFreq(420.0f);
        rate_ = samplerate;
        freq_ = 420.0f;
        drv_ = 0.005f;
        res_ = 0.5f;
        amp_ = 1.0f;
        type_ = 0;
    }

    float Process(float in)
    {
        updateFreq(freq_);
        updateRes(res_);
        updateDrv(drv_);
        filt.Process(in);

        switch (type_)
        {
        case 0:
            return filt.Low();
        case 1:
            return filt.Band();
        case 2:
            return filt.High();
        default:
            return filt.Low();
        }
    }
    void updateFreq(float freq)
    {
        if (freq > (rate_ / 2))
            freq = (rate_ / 2);
        freq_ = freq;
        filt.SetFreq(freq_);
    }
    void updateRes(float res)
    {
        res_ = res;
        filt.SetRes(res_);
    }
    void updateDrv(float drv)
    {
        if (drv > 0.1f)
            drv = 0.1f;
        drv_ = drv;
        filt.SetDrive(drv_);
    }
    void updateType(float type)
    {
        if (type <= 0.34)
        {
            type_ = 0;
        }
        else if (type > 0.34 && type <= 0.66)
        {
            type_ = 1;
        }
        else if (type > 0.66)
        {
            type_ = 2;
        }
    }
};

Filter filter;
OledDisplay display;
bool passthru;
void UpdateControls();
void UpdateDisplay();
void UpdateLeds();

void InitKnobs()
{
    knobs[0] = hw.KNOB_1;
    knobs[1] = hw.KNOB_2;
    knobs[2] = hw.KNOB_3;
    knobs[3] = hw.KNOB_4;
    knobs[4] = hw.KNOB_5;
    knobs[5] = hw.KNOB_6;
    knobs[6] = hw.KNOB_7;
    knobs[7] = hw.KNOB_8;
}

// This runs at a fixed rate, to prepare audio samples
static void AudioCallback(float **in, float **out, size_t size)
{
    UpdateControls();

    for (size_t i = 0; i < size; i++)
    {
        for (int chn = 0; chn < 2; chn++)
        {
            float sig = in[chn][i];
            sig *= 0.5f;
            sig += filter.Process(in[chn][i]);

            if (!passthru)
                out[chn][i] = sig;
            else
                out[chn][i] = in[chn][i];
        }
    }
}

int main(void)
{
    float samplerate;
    hw.Init();
    samplerate = hw.AudioSampleRate();
    filter.Init(samplerate);
    InitKnobs();
    passthru = false;

    hw.StartAdc();
    hw.StartAudio(AudioCallback);

    while (1)
    {
        UpdateLeds();
        UpdateDisplay();
        dsy_system_delay(6);
    }
}

void UpdateControls()
{
    hw.ProcessAnalogControls();
    hw.ProcessDigitalControls();

    passthru = hw.GetSwitch(hw.SW_1)->RisingEdge() ? !passthru : passthru;

    float freqVal = hw.GetKnobValue(knobs[0]);
    float resVal = hw.GetKnobValue(knobs[1]);
    float drvVal = hw.GetKnobValue(knobs[2]);
    float typeVal = hw.GetKnobValue(knobs[3]);

    if (condUpdate.Process(freqVal))
        filter.updateFreq(exp(log(20) + freqVal * (log(20000) - log(20)))); // logarithmic scale from 20Hz to 20kHz

    if (condUpdate.Process(resVal))
        filter.updateRes(resVal + 0.06); // resonance from 0.06 to 1.0, non-zero because of bug

    if (condUpdate.Process(drvVal))
        filter.updateDrv(0.001 * drvVal); // log scale

    if (condUpdate.Process(typeVal))
        filter.updateType(typeVal);
}

void UpdateDisplay()
{
    char type[8];

    for (uint8_t q = 0; q < sizeof(strbuff); q++)
    {
        strbuff[q] = 0;
    }

    sprintf(strbuff, "F:%5d", static_cast<int>(filter.freq_));
    display.SetCursor(0, 0);
    display.WriteString(strbuff, Font_7x10, true);

    sprintf(strbuff, "Q:%4d", static_cast<int>(filter.res_ * 100));
    display.SetCursor(0, 10);
    display.WriteString(strbuff, Font_7x10, true);

    sprintf(strbuff, "DRV:%3d", static_cast<int>(filter.drv_ * 10000));
    display.SetCursor(0, 20);
    display.WriteString(strbuff, Font_7x10, true);

    switch (filter.type_)
    {
    case 0:
        strncpy(type, "LOW ", sizeof(type));
        break;
    case 1:
        strncpy(type, "BAND", sizeof(type));
        break;
    case 2:
        strncpy(type, "HIGH", sizeof(type));
        break;
    }
    sprintf(strbuff, "TYP:%s", type);
    display.SetCursor(0, 30);
    display.WriteString(strbuff, Font_7x10, true);

    const char *passthruDisp = (passthru) ? "PASS" : "FILT";
    sprintf(strbuff, passthruDisp);
    display.SetCursor(0, 40);
    display.WriteString(strbuff, Font_7x10, true);
    display.Update();
}
void UpdateLeds()
{
    hw.led_driver.SetLed(DaisyField::LED_KNOB_1, hw.GetKnobValue(knobs[0]));
    hw.led_driver.SetLed(DaisyField::LED_KNOB_2, hw.GetKnobValue(knobs[1]));
    hw.led_driver.SetLed(DaisyField::LED_KNOB_3, hw.GetKnobValue(knobs[2]));
    hw.led_driver.SetLed(DaisyField::LED_KNOB_4, hw.GetKnobValue(knobs[3]));
    hw.led_driver.SwapBuffersAndTransmit();
}
