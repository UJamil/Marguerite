# Marguerite
# Electrosmith Daisy Field  Dual SVF Filterbank

## Introduction

Sometimes shaping sounds is just as important as generating sound. Filters are some of the most basic elements of signal processing. In this project I implement a dual state-variable filter (SVF) filter bank using the Electrosmith Daisy Field embedded development environment.

## Background

### State Variable Filters

State variable filters can output Low-Pass, Band-Pass and High-Pass through feedback circuits. The SVF offers the advantage of independent selection of gain, cutoff frequency (Fc) and resonance (Q) without affecting performance over other filters such as the biquadratic filter. A disadvantage of the digital implementation of the SVF is that it becomes unstable at higher frequencies. To overcome this, one method is to oversample, running the filter twice on the audio block, discarding one of the samples, effectively doubling the sample rate. Each integration is fed back through the filter and can be shown as:



In this implementation, the selection of a state-variable filter offers flexibility when choosing the filter type for processing, especially when cascading or summing the filters.  

### Electrosmith Daisy

The Electrosmith Daisy Field is a complete embedded development environment based on a Daisy seed core. The Field features audio input and output, a 16-button keyboard, 2 switches, 8 knobs, an OLED screen and various CV and MIDI controls. The field offers an accessible interface and is easy to program with community-developed example code. I chose to explore this board for my project because it aligns my interests of DSP and embedded systems programming with music hardware development. Below is the front panel of the Electrosmith Daisy Field for reference (note the small yellow Daisy Seed board as the core in the top left):


## Concept and Overview

The initial concept of this project was to develop a simple filter for the incoming audio signal. A single filter, although useful, does not add much practicality to effects processing, so a dual filter bank was designed. This dual filterbank is partially based on the Sherman Filterbank, which features two filters of selectable type (low, band, high-pass) that can be placed either in serial or parallel. In serial, the filters are cascaded, the output of the first SVF is fed to the input of the second SVF. In parallel, both filters process the audio input, and the output of each filter is then summed. The filterbank also has a pass-through feature, in which the audio input passes through without being affected by the filters. 

## Implementation

Much of the existing example codebase and libraries were developed in C++, this is also the language I am most comfortable and proficient in. To develop the dual SVF, I made two filter objects that used code from two libraries: libDaisy and DaisySP. I setup the hardware inputs for the knobs, and switches in the function UpdateControls() and the state of the LEDs in the function UpdateLeds(). To process the filters, the Daisy uses a function called AudioCallback(). The audio input is sent to the filters, and depending on the serial/parallel selection the output is either sent to the next filter or summed to the output. The OLED screen displays the relevant information for each filter: settings for cutoff frequency F, drive, resonance Q, and filter type (low, band or high). The range of cutoff frequency is set to 20Hz to 20kHz, the accepted range of human hearing. The resonance is set from 6 to 99, since the filter will crash when the cutoff frequency is set to maximum and resonance set below 6. The drive setting ranges from 0 to 9, and is only noticeable when cutoff or resonance are set to extremes. The fourth knob selects the filter output type: low pass, band pass, or high pass. 


## Results and Observations

In the current implementation, the filter does not appear to have much effect after setting the cutoff frequency higher than 18kHz, although an interesting phase distortion occurs at maximum cutoff due to the nature of the filter. Another interesting bug is that resonance in this implementation must have a minimum of 6 points since the program will crash when cutoff is set to a maximum and resonance set to below 6. This may be caused by an invalid division by zero in the SVF algorithm and also the effect of cutoff frequency being dependent on the audio sample rate. 

It is possible to create a bandpass when setting the filters to serial, the first filter to highpass, and the second filter to low pass. This may give a wider filter response since the cutoff frequencies of the band pass are individually selectable, instead of the single band pass selection using the output of low pass subtracted from high pass at that frequency. Another interesting feature is the selection of resonance that will produce distortion (clipping of signal) when set to high values, due to the increase of gain at the peak frequency of the filter. 

## Closing Thoughts

The filterbank in its current implementation serves as a powerful tool for processing audio and producing interesting effects when setting the filter parameters to various settings. The accessibility of the controls and simplicity of the design offer much capability when effecting the signal and can be used for a range of audio applications such as audio signal processing and realtime performance. The main design goal was to implement a filterbank that can be played very much like an instrument, instead of a “set-and-forget” device, which can be achieved through simple accessible controls. 

## Further Reading

Marguerite Dual SVF Filterbank Github Repository 

Stanford CCRMA Digital State Variable Filters from J.O.Smith III

Electronics Tutorials: State Variable Filter

Music DSP State Variable Filter Implementation by Andrew Simper

Electrosmith libDaisy Github repository

Electrosmith DaisySP Github repository 

Sherman Filterbank 
