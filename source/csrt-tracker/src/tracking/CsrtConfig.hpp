#pragma once

struct CsrtConfig
{
    bool useHog{true};
    bool useColorNames{true};
    bool useGray{true};
    bool useRgb{false};
    bool useChannelWeights{true};
    bool useSegmentation{true};
    float templateSize{200.0F};
    float padding{3.0F};
    int numberOfScales{33};
    float scaleStep{1.02F};
    int admmIterations{4};
    float psrThreshold{0.035F};
};
