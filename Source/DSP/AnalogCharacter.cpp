#include "AnalogCharacter.h"
#include <algorithm>

AnalogCharacter::AnalogCharacter() {}

void AnalogCharacter::prepare(double sr, int)
{
    sampleRate = sr;
    resetState();
    modPhase = 0.0f;
    delayIdx = 0;
    std::fill(delayBuf, delayBuf + DelayLen, 0.0f);

    float fs = (float)sr;
    if (wdfEngine) wdfEngine->prepare(fs);
    if (wdfEngine2) wdfEngine2->prepare(fs);
}

void AnalogCharacter::resetState()
{
    for (int i = 0; i < NumFilters; ++i) filters[i].reset();
    inputSatFilter.reset();
    inputStage.reset();
    lowStage.reset();
    midStage.reset();
    highStage.reset();
    outputStage.reset();
    psmEnvelope = 0.0f;
}

void AnalogCharacter::setType(Type t)
{
    if (currentType == t) return;
    currentType = t;
    loadPreset();
}

// ==================== BIQUAD ====================

void AnalogCharacter::Biquad::setLowShelf(float freq, float gainDB, float q, float sr)
{
    if (std::abs(gainDB) < 0.01f) { setPassthrough(); return; }
    float A = std::pow(10.0f, gainDB / 40.0f);
    float w0 = juce::MathConstants<float>::twoPi * freq / sr;
    float alpha = std::sin(w0) / (2.0f * q);
    float cosW = std::cos(w0);
    float sqrtA2 = 2.0f * std::sqrt(A) * alpha;
    float a0 = (A + 1.0f) + (A - 1.0f) * cosW + sqrtA2;
    b0 = A * ((A + 1.0f) - (A - 1.0f) * cosW + sqrtA2) / a0;
    b1 = 2.0f * A * ((A - 1.0f) - (A + 1.0f) * cosW) / a0;
    b2 = A * ((A + 1.0f) - (A - 1.0f) * cosW - sqrtA2) / a0;
    a1 = -2.0f * ((A - 1.0f) + (A + 1.0f) * cosW) / a0;
    a2 = ((A + 1.0f) + (A - 1.0f) * cosW - sqrtA2) / a0;
}

void AnalogCharacter::Biquad::setHighShelf(float freq, float gainDB, float q, float sr)
{
    if (std::abs(gainDB) < 0.01f) { setPassthrough(); return; }
    float A = std::pow(10.0f, gainDB / 40.0f);
    float w0 = juce::MathConstants<float>::twoPi * freq / sr;
    float alpha = std::sin(w0) / (2.0f * q);
    float cosW = std::cos(w0);
    float sqrtA2 = 2.0f * std::sqrt(A) * alpha;
    float a0 = (A + 1.0f) - (A - 1.0f) * cosW + sqrtA2;
    b0 = A * ((A + 1.0f) + (A - 1.0f) * cosW + sqrtA2) / a0;
    b1 = -2.0f * A * ((A - 1.0f) + (A + 1.0f) * cosW) / a0;
    b2 = A * ((A + 1.0f) + (A - 1.0f) * cosW - sqrtA2) / a0;
    a1 = 2.0f * ((A - 1.0f) - (A + 1.0f) * cosW) / a0;
    a2 = ((A + 1.0f) - (A - 1.0f) * cosW - sqrtA2) / a0;
}

void AnalogCharacter::Biquad::setPeak(float freq, float gainDB, float q, float sr)
{
    if (std::abs(gainDB) < 0.01f) { setPassthrough(); return; }
    float A = std::pow(10.0f, gainDB / 40.0f);
    float w0 = juce::MathConstants<float>::twoPi * freq / sr;
    float alpha = std::sin(w0) / (2.0f * q);
    float cosW = std::cos(w0);
    float a0 = 1.0f + alpha / A;
    b0 = (1.0f + alpha * A) / a0;
    b1 = (-2.0f * cosW) / a0;
    b2 = (1.0f - alpha * A) / a0;
    a1 = (2.0f * cosW) / a0;
    a2 = (-1.0f + alpha / A) / a0;
}

void AnalogCharacter::Biquad::setHighCut(float freq, float q, float sr)
{
    float w0 = juce::MathConstants<float>::twoPi * freq / sr;
    float alpha = std::sin(w0) / (2.0f * q);
    float cosW = std::cos(w0);
    float a0 = 1.0f + alpha;
    b0 = (1.0f - cosW) / (2.0f * a0);
    b1 = (1.0f - cosW) / a0;
    b2 = (1.0f - cosW) / (2.0f * a0);
    a1 = (-2.0f * cosW) / a0;
    a2 = (1.0f - alpha) / a0;
}

void AnalogCharacter::Biquad::setLowCut(float freq, float q, float sr)
{
    float w0 = juce::MathConstants<float>::twoPi * freq / sr;
    float alpha = std::sin(w0) / (2.0f * q);
    float cosW = std::cos(w0);
    float a0 = 1.0f + alpha;
    b0 = (1.0f + cosW) / (2.0f * a0);
    b1 = -(1.0f + cosW) / a0;
    b2 = (1.0f + cosW) / (2.0f * a0);
    a1 = (-2.0f * cosW) / a0;
    a2 = (1.0f - alpha) / a0;
}

void AnalogCharacter::Biquad::setPassthrough()
{
    b0 = 1.0f; b1 = 0.0f; b2 = 0.0f;
    a1 = 0.0f; a2 = 0.0f;
}

// ==================== CIRCUIT STAGE ====================

void AnalogCharacter::CircuitStage::setTimes(float atkMs, float relMs, float sr)
{
    attackCoeff = 1.0f - std::exp(-1.0f / (atkMs * 0.001f * (float)sr));
    releaseCoeff = 1.0f - std::exp(-1.0f / (relMs * 0.001f * (float)sr));
}

void AnalogCharacter::CircuitStage::reset()
{
    envelope = 0.0f; slewState = 0.0f;
    gridCharge = 0.0f; coreFlux = 0.0f; prevIn = 0.0f;
    capMemLow = 0.0f; capMemMid = 0.0f;
    loading = 0.0f;
}

float AnalogCharacter::CircuitStage::process(float x, int type, float drive, float psmGain, float loadFromPrev)
{
    // Envelope follower (RMS-sensed, like real analog)
    float absX = std::abs(x);
    float diff = absX - envelope;
    if (diff > 0.0f) envelope += attackCoeff * diff;
    else envelope += releaseCoeff * diff;
    envelope = juce::jmax(0.0f, envelope);

    // Effective drive = base × power supply sag
    float effectiveDrive = drive * psmGain;

    // Boost drive from envelope (bias shift)
    effectiveDrive *= (1.0f + envelope * 1.5f);

    // ===== GRID CURRENT RECTIFICATION (tube types) =====
    // When signal pushes grid positive, grid conducts current
    // This charges the coupling cap, shifting bias down (compression)
    if (type == 0)
    {
        if (x > 0.3f)
        {
            float gridCurrent = (x - 0.3f) * 0.003f;
            gridCharge += gridCurrent;
        }
        gridCharge *= gridLeak; // slow discharge through grid resistor
        x -= gridCharge * 0.4f;
    }

    // ===== TRANSFORMER HYSTERESIS (xfmr types) =====
    // Core retains magnetization from previous peaks
    // Energy lost to hysteresis = area of BH loop
    if (type == 1)
    {
        float flux = x * 0.15f + coreFlux * 0.85f;
        float hystLoss = (flux - coreFlux) * 0.08f;
        x -= hystLoss;
        coreFlux = flux;
    }

    // ===== DIELECTRIC ABSORPTION (all types) =====
    // Capacitors have molecular memory — they remember past voltages
    // Two time constants: fast (polypropylene) and slow (electrolytic)
    capMemLow = capMemLow * 0.9999f + x * 0.0001f;
    capMemMid = capMemMid * 0.999f + x * 0.001f;
    x += (capMemLow + capMemMid) * 0.0005f;

    // ===== INTER-STAGE LOADING =====
    // When previous stage drives harder, this stage's effective impedance changes
    // Models passive filter stage loading (e.g., LC filter impedance varies with level)
    float loadMod = 1.0f - loadFromPrev * 0.2f;
    effectiveDrive *= loadMod;

    // ===== SLEW RATE LIMITING =====
    // Real components can't change voltage instantly
    // Slew rate decreases as signal level increases (opamp bandwidth compression)
    float slewLimit = 1.0f / (1.0f + envelope * 6.0f);
    float slew = (x - slewState) * slewLimit;
    if (std::abs(slew) > 0.3f) slew = (slew > 0 ? 0.3f : -0.3f);
    slewState = slewState + slew;
    float limited = slewState;

    // ===== SATURATE =====
    float result = 0.0f;
    switch (type)
    {
        case 0: // Tube
        {
            float d = effectiveDrive * 1.5f;
            float y = limited * d;
            float pos = y > 0 ? y / (1.0f + y * 0.55f) : 0;
            float neg = y < 0 ? y / (1.0f + std::abs(y) * 0.75f) : 0;
            result = pos + neg;
            result += 0.06f * result * result;
            result += 0.015f * result * result * result;
            float norm = (d > 0.1f) ? (1.0f / std::tanh(d * 0.8f)) : 1.0f;
            result *= norm * 0.92f;
            break;
        }
        case 1: // Transformer
        {
            float d = effectiveDrive * 1.2f;
            float y = limited * d;
            float ySq = y * y;
            float core = y / (1.0f + ySq * 0.6f);
            float harm = (y * ySq) * 0.08f + (y * ySq * ySq) * 0.02f;
            result = core + harm;
            float norm = (d > 0.1f) ? (1.0f / (std::tanh(d * 0.6f) * 1.15f)) : 1.0f;
            result *= norm;
            break;
        }
        case 2: // VCA
        {
            float d = effectiveDrive * 1.0f;
            float y = limited * d;
            float thresh = 0.45f;
            float absY = std::abs(y);
            if (absY <= thresh)
            {
                result = y * 0.85f;
                float norm = d > 0.1f ? (1.0f / d) * 0.85f : 1.0f;
                result /= norm;
            }
            else
            {
                float over = (absY - thresh) / (1.0f - thresh);
                float comp = thresh + (1.0f - thresh) * std::atan(over * 2.0f) / (juce::MathConstants<float>::pi / 2.0f);
                result = (y > 0 ? comp : -comp);
                result += 0.02f * result * result * result;
                float norm = d > 0.1f ? (1.0f / d) * 0.85f : 1.0f;
                result /= norm;
            }
            break;
        }
        case 3: // Opamp
        {
            float d = effectiveDrive * 1.3f;
            float y = limited * d;
            result = std::tanh(y * 1.8f);
            result = result * 0.82f + 0.12f * (4.0f * result * result * result - 3.0f * result);
            result += 0.06f * std::tanh(y * 4.0f);
            float norm = (d > 0.1f) ? (1.0f / std::tanh(d * 1.4f)) : 1.0f;
            result *= norm * 0.88f;
            break;
        }
        default: result = limited;
    }

    // ===== BIAS RECOVERY GAIN MAKEUP =====
    // When signal compresses the stage, the gain stage recovers
    // This is the "pump" of analog compression
    float makeup = 1.0f + envelope * 0.08f * releaseCoeff * 100.0f;
    result *= (1.0f + envelope * 0.12f);

    // Store loading for next stage
    loading = envelope;

    prevIn = x;
    return result;
}

// ==================== STATIC SAT HELPERS ====================

float AnalogCharacter::saturateTube(float x, float drive) const
{
    if (drive < 0.001f) return x;
    float d = drive * 1.5f;
    float y = x * d;
    float pos = y > 0 ? y / (1.0f + y * 0.55f) : 0;
    float neg = y < 0 ? y / (1.0f + std::abs(y) * 0.75f) : 0;
    float out = pos + neg;
    out += 0.06f * out * out;
    out += 0.015f * out * out * out;
    float norm = (d > 0.1f) ? (1.0f / std::tanh(d * 0.7f)) : 1.0f;
    return out * norm * 0.95f;
}

float AnalogCharacter::saturateTransformer(float x, float drive) const
{
    if (drive < 0.001f) return x;
    float d = drive * 1.2f;
    float y = x * d;
    float ySq = y * y;
    float core = y / (1.0f + ySq * 0.6f);
    float harm = (y * ySq) * 0.08f + (y * ySq * ySq) * 0.02f;
    float out = core + harm;
    float norm = (d > 0.1f) ? (1.0f / (std::tanh(d * 0.6f) * 1.1f)) : 1.0f;
    return out * norm;
}

float AnalogCharacter::saturateVCA(float x, float drive) const
{
    if (drive < 0.001f) return x;
    float d = drive * 1.0f;
    float y = x * d;
    float thresh = 0.45f;
    float absY = std::abs(y);
    if (absY <= thresh)
    {
        float out = y * 0.85f;
        float norm = d > 0.1f ? (1.0f / d) * 0.85f : 1.0f;
        return out / norm;
    }
    float over = (absY - thresh) / (1.0f - thresh);
    float comp = thresh + (1.0f - thresh) * std::atan(over * 2.0f) / (juce::MathConstants<float>::pi / 2.0f);
    float out = (y > 0 ? comp : -comp);
    out += 0.02f * out * out * out;
    float norm = d > 0.1f ? (1.0f / d) * 0.85f : 1.0f;
    return out / norm;
}

float AnalogCharacter::saturateOpamp(float x, float drive) const
{
    if (drive < 0.001f) return x;
    float d = drive * 1.3f;
    float y = x * d;
    float out = std::tanh(y * 1.8f);
    out = out * 0.82f + 0.12f * (4.0f * out * out * out - 3.0f * out);
    out += 0.06f * std::tanh(y * 4.0f);
    float norm = (d > 0.1f) ? (1.0f / std::tanh(d * 1.2f)) : 1.0f;
    return out * norm * 0.9f;
}

float AnalogCharacter::saturate(int type, float x, float drive) const
{
    switch (type) {
        case 0: return saturateTube(x, drive);
        case 1: return saturateTransformer(x, drive);
        case 2: return saturateVCA(x, drive);
        case 3: return saturateOpamp(x, drive);
        default: return x;
    }
}

// ==================== PRESET LOADER ====================

void AnalogCharacter::loadPreset()
{
    resetState();
    for (int i = 0; i < NumFilters; ++i) filters[i].setPassthrough();
    inputSatFilter.setPassthrough();
    filterCount = 0;

    psmAttack = 0.01f; psmRelease = 0.0003f; psmDepth = 0.03f;
    loadingAmount = 0.0f;
    imdAmount = 0.0f;
    modAmount = 0.0f;

    useWDF = false;
    wdfMakeupGain = 1.0f;

    auto setAll = [&](CircuitStage& s, float atk, float rel, float drive, int type, float leak = 0.999f) {
        s.setTimes(atk, rel, (float)sampleRate);
        s.baseDrive = drive;
        s.satType = type;
        s.gridLeak = leak;
        s.reset();
    };

    float sr = (float)sampleRate;

    switch (currentType)
    {
        // ===== FORMPASSIVE (Mister Passive) - WDF LC Circuit =====
        case FormPassive:
        {
            useWDF = true;
            wdfMakeupGain = 40.0f;
            wdfEngine = createWDFEngine(WDF_MisterPassive, sr);
            wdfEngine2 = createWDFEngine(WDF_MisterPassive, sr);
            wdfEngine2->randomizeTolerance(wdfTolerance, 42);

            filterCount = 0;
            for (int i = 0; i < NumFilters; ++i) filters[i].setPassthrough();
            inputSatFilter.setPassthrough();

            setAll(inputStage, 30.0f, 250.0f, 0.12f, 0, 0.998f);
            setAll(lowStage,   10.0f, 120.0f, 0.30f, 0, 0.997f);
            setAll(midStage,    5.0f,  70.0f, 0.20f, 0, 0.998f);
            setAll(highStage,   2.0f,  40.0f, 0.15f, 0, 0.999f);
            setAll(outputStage,20.0f, 180.0f, 0.10f, 0, 0.998f);

            psmAttack = 0.008f; psmRelease = 0.0002f; psmDepth = 0.04f;
            loadingAmount = 0.6f;
            imdAmount = 0.010f;
            modAmount = 0.0003f;
            break;
        }

        // ===== CRANESONG (Krane Mybiz) - WDF Active Parametric =====
        case CraneSong:
        {
            useWDF = true;
            wdfMakeupGain = 35.0f;
            wdfEngine = createWDFEngine(WDF_KraneMybiz, sr);
            wdfEngine2 = createWDFEngine(WDF_KraneMybiz, sr);
            wdfEngine2->randomizeTolerance(wdfTolerance, 43);

            filterCount = 0;
            for (int i = 0; i < NumFilters; ++i) filters[i].setPassthrough();
            inputSatFilter.setPassthrough();

            setAll(inputStage, 20.0f, 200.0f, 0.10f, 0, 0.998f);
            setAll(lowStage,   50.0f, 350.0f, 0.08f, 1);
            setAll(midStage,   12.0f, 100.0f, 0.10f, 1);
            setAll(highStage,   3.0f,  25.0f, 0.06f, 1);
            setAll(outputStage,30.0f, 250.0f, 0.08f, 1);

            psmAttack = 0.02f; psmRelease = 0.0005f; psmDepth = 0.015f;
            loadingAmount = 0.3f;
            imdAmount = 0.006f;
            modAmount = 0.0001f;
            break;
        }

        // ===== VALVETUBE (West Nugget) - WDF Tube EQ =====
        case ValveTube:
        {
            useWDF = true;
            wdfMakeupGain = 18.0f;
            wdfEngine = createWDFEngine(WDF_WestNugget, sr);
            wdfEngine2 = createWDFEngine(WDF_WestNugget, sr);
            wdfEngine2->randomizeTolerance(wdfTolerance, 44);

            filterCount = 0;
            for (int i = 0; i < NumFilters; ++i) filters[i].setPassthrough();
            inputSatFilter.setPassthrough();

            setAll(inputStage, 25.0f, 180.0f, 0.20f, 0, 0.996f);
            setAll(lowStage,    8.0f,  90.0f, 0.50f, 0, 0.995f);
            setAll(midStage,    4.0f,  55.0f, 0.60f, 0, 0.994f);
            setAll(highStage,   1.5f,  20.0f, 0.30f, 0, 0.997f);
            setAll(outputStage,15.0f, 140.0f, 0.25f, 0, 0.996f);

            psmAttack = 0.005f; psmRelease = 0.00015f; psmDepth = 0.05f;
            loadingAmount = 0.7f;
            imdAmount = 0.015f;
            modAmount = 0.0004f;
            break;
        }

        // ===== PULSETEQ (Pool Dake) - WDF Passive LC =====
        case PulsetEQ:
        {
            useWDF = true;
            wdfMakeupGain = 40.0f;
            wdfEngine = createWDFEngine(WDF_PoolDake, sr);
            wdfEngine2 = createWDFEngine(WDF_PoolDake, sr);
            wdfEngine2->randomizeTolerance(wdfTolerance, 45);

            filterCount = 0;
            for (int i = 0; i < NumFilters; ++i) filters[i].setPassthrough();
            inputSatFilter.setPassthrough();

            setAll(inputStage, 30.0f, 220.0f, 0.12f, 0, 0.998f);
            setAll(lowStage,   15.0f, 140.0f, 0.30f, 0, 0.997f);
            setAll(midStage,    6.0f,  65.0f, 0.20f, 0, 0.998f);
            setAll(highStage,   2.5f,  25.0f, 0.18f, 0, 0.999f);
            setAll(outputStage,20.0f, 180.0f, 0.12f, 0, 0.998f);

            psmAttack = 0.007f; psmRelease = 0.00018f; psmDepth = 0.035f;
            loadingAmount = 0.5f;
            imdAmount = 0.008f;
            modAmount = 0.0002f;
            break;
        }

        // ===== CONSOLE88 (Never 80-8) - WDF Inductor EQ =====
        case Console88:
        {
            useWDF = true;
            wdfMakeupGain = 35.0f;
            wdfEngine = createWDFEngine(WDF_Never80_8, sr);
            wdfEngine2 = createWDFEngine(WDF_Never80_8, sr);
            wdfEngine2->randomizeTolerance(wdfTolerance, 46);

            filterCount = 0;
            for (int i = 0; i < NumFilters; ++i) filters[i].setPassthrough();
            inputSatFilter.setPassthrough();

            setAll(inputStage, 35.0f, 300.0f, 0.18f, 0, 0.998f);
            setAll(lowStage,   45.0f, 250.0f, 0.28f, 1);
            setAll(midStage,    8.0f,  70.0f, 0.35f, 3);
            setAll(highStage,   1.5f,  12.0f, 0.15f, 3);
            setAll(outputStage,25.0f, 220.0f, 0.25f, 1);

            psmAttack = 0.01f; psmRelease = 0.00025f; psmDepth = 0.03f;
            loadingAmount = 0.55f;
            imdAmount = 0.012f;
            modAmount = 0.00018f;
            break;
        }

        // ===== GBUS (Liquid State Solid) - WDF Active RC =====
        case GBus:
        {
            useWDF = true;
            wdfMakeupGain = 50.0f;
            wdfEngine = createWDFEngine(WDF_LiquidStateSolid, sr);
            wdfEngine2 = createWDFEngine(WDF_LiquidStateSolid, sr);
            wdfEngine2->randomizeTolerance(wdfTolerance, 47);

            filterCount = 0;
            for (int i = 0; i < NumFilters; ++i) filters[i].setPassthrough();
            inputSatFilter.setPassthrough();

            setAll(inputStage, 12.0f, 60.0f, 0.20f, 2);
            setAll(lowStage,    5.0f,  35.0f, 0.40f, 2);
            setAll(midStage,    2.5f,  20.0f, 0.55f, 2);
            setAll(highStage,   0.8f,   6.0f, 0.30f, 3);
            setAll(outputStage,10.0f,  50.0f, 0.20f, 2);

            psmAttack = 0.004f; psmRelease = 0.0001f; psmDepth = 0.045f;
            loadingAmount = 0.65f;
            imdAmount = 0.018f;
            modAmount = 0.00015f;
            break;
        }

        default: break;
    }
}

// ==================== MAIN PROCESSING ====================

void AnalogCharacter::process(float* data, int numSamples, int channel)
{
    if (currentType == Off) return;

    float sr = (float)sampleRate;
    float modInc = juce::MathConstants<float>::twoPi * 0.15f / sr;
    auto& wdf = (channel == 0 && wdfEngine) ? *wdfEngine
               : (wdfEngine2 ? *wdfEngine2 : *wdfEngine);

    if (useWDF && wdfEngine)
    {
        for (int s = 0; s < numSamples; ++s)
        {
            float x = data[s];

            // ===== WDF PASSIVE EQ (4-band LC circuit) =====
            x = wdf.process(x);
            x *= wdfMakeupGain; // compensate passive insertion loss

            // ===== POWER SUPPLY MODULATION =====
            float absX = std::abs(x);
            float psmDiff = absX - psmEnvelope;
            if (psmDiff > 0) psmEnvelope += psmAttack * psmDiff;
            else psmEnvelope += psmRelease * psmDiff;
            psmEnvelope = juce::jmax(0.0f, juce::jmin(1.0f, psmEnvelope));
            float psmGain = 1.0f - psmEnvelope * psmDepth;

            // ===== OUTPUT SATURATION (makeup stage character) =====
            x = outputStage.process(x, outputStage.satType, outputStage.baseDrive, psmGain, 0.0f);

            // ===== INTERMODULATION =====
            int prev = (delayIdx - 1 + DelayLen) % DelayLen;
            float imd = delayBuf[prev] * x * imdAmount;
            delayBuf[delayIdx] = x;
            delayIdx = (delayIdx + 1) % DelayLen;
            x += imd * 3.0f;

            // ===== BIAS MODULATION =====
            modPhase += modInc;
            if (modPhase > juce::MathConstants<float>::twoPi)
                modPhase -= juce::MathConstants<float>::twoPi;
            float mod = 1.0f + modAmount * std::sin(modPhase);
            x *= mod;

            data[s] = x;
        }
    }
    else
    {
        for (int s = 0; s < numSamples; ++s)
        {
            float x = data[s];

            // ===== POWER SUPPLY MODULATION =====
            float absX = std::abs(x);
            float psmDiff = absX - psmEnvelope;
            if (psmDiff > 0) psmEnvelope += psmAttack * psmDiff;
            else psmEnvelope += psmRelease * psmDiff;
            psmEnvelope = juce::jmax(0.0f, juce::jmin(1.0f, psmEnvelope));
            float psmGain = 1.0f - psmEnvelope * psmDepth;

            // ===== INPUT STAGE =====
            x = inputSatFilter.process(x);
            x = inputStage.process(x, inputStage.satType, inputStage.baseDrive, psmGain, 0.0f);

            // ===== FILTER GROUP 1 (Lows) =====
            float loadFromPrev = inputStage.loading * loadingAmount;
            if (filterCount > 0)
            {
                int end = juce::jmin(3, filterCount);
                for (int i = 0; i < end; ++i)
                    x = filters[i].process(x);
                x = lowStage.process(x, lowStage.satType, lowStage.baseDrive, psmGain, loadFromPrev);
            }

            // ===== FILTER GROUP 2 (Mids) =====
            loadFromPrev = lowStage.loading * loadingAmount;
            if (filterCount > 3)
            {
                int end = juce::jmin(5, filterCount);
                for (int i = 3; i < end; ++i)
                    x = filters[i].process(x);
                x = midStage.process(x, midStage.satType, midStage.baseDrive, psmGain, loadFromPrev);
            }

            // ===== FILTER GROUP 3 (Highs) =====
            loadFromPrev = midStage.loading * loadingAmount;
            if (filterCount > 5)
            {
                for (int i = 5; i < filterCount; ++i)
                    x = filters[i].process(x);
                x = highStage.process(x, highStage.satType, highStage.baseDrive, psmGain, loadFromPrev);
            }

            // ===== OUTPUT STAGE =====
            loadFromPrev = highStage.loading * loadingAmount;
            x = outputStage.process(x, outputStage.satType, outputStage.baseDrive, psmGain, loadFromPrev);

            // ===== INTERMODULATION =====
            int prev = (delayIdx - 1 + DelayLen) % DelayLen;
            float imd = delayBuf[prev] * x * imdAmount;
            delayBuf[delayIdx] = x;
            delayIdx = (delayIdx + 1) % DelayLen;
            x += imd * 3.0f;

            // ===== BIAS MODULATION =====
            modPhase += modInc;
            if (modPhase > juce::MathConstants<float>::twoPi)
                modPhase -= juce::MathConstants<float>::twoPi;
            float mod = 1.0f + modAmount * std::sin(modPhase);
            x *= mod;

            data[s] = x;
        }
    }
}

float AnalogCharacter::getFrequencyResponse(float freq) const
{
    if (currentType == Off) return 0.0f;
    float sr = (float)sampleRate;
    float w = juce::MathConstants<float>::twoPi * freq / sr;
    auto eval = [&](const Biquad& f) -> float {
        float c1 = std::cos(w), c2 = std::cos(2.0f * w);
        float s1 = std::sin(w), s2 = std::sin(2.0f * w);
        float reNum = f.b0 + f.b1 * c1 + f.b2 * c2;
        float imNum = -f.b1 * s1 - f.b2 * s2;
        float reDen = 1.0f + f.a1 * c1 + f.a2 * c2;
        float imDen = -f.a1 * s1 - f.a2 * s2;
        return 10.0f * std::log10(std::max(1e-10f, (reNum*reNum+imNum*imNum)/(reDen*reDen+imDen*imDen)));
    };
    float total = 0.0f;
    total += eval(inputSatFilter);
    for (int i = 0; i < filterCount; ++i) total += eval(filters[i]);
    return total;
}
