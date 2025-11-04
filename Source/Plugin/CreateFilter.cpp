// Separate compilation unit for JUCE plugin factory
#include "PluginProcessor.h"
#include <JuceHeader.h>

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new TitanVocalProcessor();
}