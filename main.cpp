#include "daisy_field.h"
#include "daisysp.h" // Uncomment this if you want to use the DSP library.
#include <stdio.h>
#include <string>

using namespace daisy;
using namespace daisysp;

#define NUM_FILTERS 2

// Declarations
class DaisyField hw;

uint8_t knobs[8];
char strbuff[128];

struct ConditionalUpdate
{
    float oldVal = 0;
    bool Process(float newVal)
    {
        if (abs(newVal - oldVal) > 0.01f)
        {
            oldVal = newVal;
            return true;
        }
        return false;
    }
};

ConditionalUpdate condUpdates[8];

struct Filter
{
    Svf filt;
    float amp_, drv_, freq_, res_, rate_;
    uint8_t type_;

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

// Filter filter1, filter2;
Filter filter[2];
OledDisplay display;
bool passthru, filt2;
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
        for (uint8_t chn = 0; chn < 2; chn++)
        {
            float sig = 0.f;
            // float sig = in[chn][i];
            // sig *= 0.5f;
            float temp = 0.f;
            sig += filter[0].Process(in[chn][i]);
            temp = sig;

            if (filt2)
            {
                sig += filter[1].Process(in[chn][i]);
            }
            else if (!filt2)
            {
                sig = filter[1].Process(temp);
            }

            // sig *= 0.5f;

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
    filter[0].Init(samplerate);
    filter[1].Init(samplerate);
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
    filt2 = hw.GetSwitch(hw.SW_2)->RisingEdge() ? !filt2 : filt2;

    float freqVal[2], resVal[2], drvVal[2], typeVal[2];
    for (uint8_t i = 0; i < NUM_FILTERS; i++)
    {
        freqVal[i] = hw.GetKnobValue(knobs[(i % 2) * 4]);
        resVal[i] = hw.GetKnobValue(knobs[((i % 2) * 4) + 1]);
        drvVal[i] = hw.GetKnobValue(knobs[((i % 2) * 4) + 2]);
        typeVal[i] = hw.GetKnobValue(knobs[((i % 2) * 4) + 3]);

        // if (condUpdates[(i % 2) * 4].Process(freqVal[i]))
        filter[i].updateFreq(exp(log(20) + freqVal[i] * (log(20000) - log(20)))); // logarithmic scale from 20Hz to 20kHz

        if (condUpdates[((i % 2) * 4) + 1].Process(resVal[i]))
            filter[i].updateRes(resVal[i] * (1.0 - 0.06) + 0.06); // resonance from 0.06 to 1.0, non-zero because of bug

        if (condUpdates[((i % 2) * 4) + 2].Process(drvVal[i]))
            filter[i].updateDrv(0.001 * drvVal[i]);

        if (condUpdates[((i % 2) * 4) + 3].Process(typeVal[i]))
            filter[i].updateType(typeVal[i]);
    }
}

void UpdateDisplay()
{
    char type1[8];
    char type2[8];

    for (uint8_t q = 0; q < sizeof(strbuff); q++)
    {
        strbuff[q] = 0;
    }

    sprintf(strbuff, "F1:%5d  F2:%5d", static_cast<int>(round(filter[0].freq_)), static_cast<int>(round(filter[1].freq_)));
    display.SetCursor(0, 0);
    display.WriteString(strbuff, Font_7x10, true);

    sprintf(strbuff, "Q1:%4d   Q2:%4d", static_cast<int>(filter[0].res_ * 100), static_cast<int>(filter[1].res_ * 100));
    display.SetCursor(0, 10);
    display.WriteString(strbuff, Font_7x10, true);

    sprintf(strbuff, "DRV1:%3d  DRV2:%3d", static_cast<int>(filter[0].drv_ * 10000), static_cast<int>(filter[1].drv_ * 10000));
    display.SetCursor(0, 20);
    display.WriteString(strbuff, Font_7x10, true);

    switch (filter[0].type_)
    {
    case 0:
        strncpy(type1, "LOW ", sizeof(type1));
        break;
    case 1:
        strncpy(type1, "BAND", sizeof(type1));
        break;
    case 2:
        strncpy(type1, "HIGH", sizeof(type1));
        break;
    }
    switch (filter[1].type_)
    {
    case 0:
        strncpy(type2, "LOW ", sizeof(type2));
        break;
    case 1:
        strncpy(type2, "BAND", sizeof(type2));
        break;
    case 2:
        strncpy(type2, "HIGH", sizeof(type2));
        break;
    }
    sprintf(strbuff, "TYP1:%s TYP2:%s", type1, type2);
    display.SetCursor(0, 30);
    display.WriteString(strbuff, Font_7x10, true);

    const char *passthruDisp = (passthru) ? "PASS" : "FILT";
    const char *filt2Disp = (filt2) ? "PAR" : "SER";
    sprintf(strbuff, "%s %s", passthruDisp, filt2Disp);
    display.SetCursor(0, 40);
    display.WriteString(strbuff, Font_7x10, false);
    display.Update();
}
void UpdateLeds()
{
    hw.led_driver.SetLed(DaisyField::LED_KNOB_1, hw.GetKnobValue(knobs[0]));
    hw.led_driver.SetLed(DaisyField::LED_KNOB_2, hw.GetKnobValue(knobs[1]));
    hw.led_driver.SetLed(DaisyField::LED_KNOB_3, hw.GetKnobValue(knobs[2]));
    hw.led_driver.SetLed(DaisyField::LED_KNOB_4, hw.GetKnobValue(knobs[3]));
    hw.led_driver.SetLed(DaisyField::LED_KNOB_5, hw.GetKnobValue(knobs[4]));
    hw.led_driver.SetLed(DaisyField::LED_KNOB_6, hw.GetKnobValue(knobs[5]));
    hw.led_driver.SetLed(DaisyField::LED_KNOB_7, hw.GetKnobValue(knobs[6]));
    hw.led_driver.SetLed(DaisyField::LED_KNOB_8, hw.GetKnobValue(knobs[7]));
    hw.led_driver.SwapBuffersAndTransmit();
}
