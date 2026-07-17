#pragma once
#include "WDFCore.h"
#include <random>
#include <memory>
#include <vector>

// ================================================================
// WDF Resonant Band — Vin -> Rsrc -> (Rdamp || L || C) -> GND
// Output = voltage across the parallel LC
// ================================================================
class WDFResonantBand {
public:
    wdf::Resistor rSource, rDamp;
    wdf::Inductor inductor;
    wdf::Capacitor capacitor;
    wdf::ParallelAdaptor lcParallel;
    wdf::ParallelAdaptor rlcParallel;
    wdf::SeriesAdaptor fullSeries;
    wdf::IdealVoltageSource vin;

    float L_val, C_val;

    WDFResonantBand(float rSrc, float rDampVal, float lVal, float cVal, float fs)
        : rSource(rSrc), rDamp(rDampVal), inductor(lVal, fs), capacitor(cVal, fs),
          lcParallel(inductor, capacitor), rlcParallel(rDamp, lcParallel),
          fullSeries(rSource, rlcParallel), vin(fullSeries),
          L_val(lVal), C_val(cVal) {}

    void prepare(float fs) {
        inductor.prepare(fs);
        capacitor.prepare(fs);
    }

    float process(float input) {
        vin.setVoltage(input);
        vin.incident(fullSeries.reflected());
        fullSeries.incident(vin.reflected());
        return rlcParallel.portVoltage();
    }

    void setFreq(float f) {
        float w = 6.283185307179586f * f;
        if (w > 1e-3f && L_val > 1e-12f) {
            C_val = 1.0f / (L_val * w * w);
            capacitor.setValue(C_val);
        }
    }

    void setQ(float q) {
        if (q < 0.1f) q = 0.1f;
        float w0 = getFreq() * 6.283185307179586f;
        if (w0 > 1e-3f) rDamp.setValue(w0 * L_val * q);
    }

    void setComponents(float l, float c) {
        L_val = l; C_val = c;
        inductor.setValue(l); capacitor.setValue(c);
    }

    void setRs(float r) { rSource.setValue(r); }

    float getFreq() const {
        if (L_val < 1e-12f || C_val < 1e-12f) return 0.0f;
        return 1.0f / (6.283185307179586f * std::sqrt(L_val * C_val));
    }
};

// ================================================================
// Base engine + all character WDF engines
// ================================================================
class WDFCharacterEngine {
public:
    virtual ~WDFCharacterEngine() = default;
    virtual void prepare(float sampleRate) = 0;
    virtual float process(float input) = 0;
    virtual void randomizeTolerance(float tolPercent, int seed) = 0;
};

// ----- Mister Passive (4-band LC) -----
class MisterPassiveWDF : public WDFCharacterEngine {
public:
    WDFResonantBand band1, band2, band3, band4;
    MisterPassiveWDF(float fs)
        : band1(50, 10000, 8.0f, 2.2e-6f, fs),
          band2(50, 6800, 1.5f, 1.0e-6f, fs),
          band3(50, 4700, 0.5f, 470e-9f, fs),
          band4(50, 3300, 0.1f, 100e-9f, fs) {}
    void prepare(float fs) override { band1.prepare(fs); band2.prepare(fs); band3.prepare(fs); band4.prepare(fs); }
    float process(float input) override {
        input = band1.process(input); input = band2.process(input);
        input = band3.process(input); input = band4.process(input);
        return input;
    }
    void randomizeTolerance(float t, int s) override {
        std::mt19937 rng(s); std::uniform_real_distribution<float> d(1.0f - t/100.0f, 1.0f + t/100.0f);
        auto rnd = [&](float v) { return v * d(rng); };
        band1.setComponents(rnd(band1.L_val), rnd(band1.C_val));
        band2.setComponents(rnd(band2.L_val), rnd(band2.C_val));
        band3.setComponents(rnd(band3.L_val), rnd(band3.C_val));
        band4.setComponents(rnd(band4.L_val), rnd(band4.C_val));
    }
};

// ----- Pool Dake (4-band Pultec-style boost/cut) -----
class PoolDakeWDF : public WDFCharacterEngine {
public:
    WDFResonantBand lowBoost, lowCut, hiBoost, hiCut;
    PoolDakeWDF(float fs)
        : lowBoost(50, 10000, 8.0f, 2.2e-6f, fs), lowCut(50, 6800, 1.5f, 1.0e-6f, fs),
          hiBoost(50, 4700, 0.5f, 470e-9f, fs), hiCut(50, 3300, 0.1f, 100e-9f, fs) {}
    void prepare(float fs) override { lowBoost.prepare(fs); lowCut.prepare(fs); hiBoost.prepare(fs); hiCut.prepare(fs); }
    float process(float input) override {
        input = lowBoost.process(input); input = lowCut.process(input);
        input = hiBoost.process(input); input = hiCut.process(input);
        return input;
    }
    void randomizeTolerance(float t, int s) override {
        std::mt19937 rng(s); std::uniform_real_distribution<float> d(1.0f - t/100.0f, 1.0f + t/100.0f);
        auto rnd = [&](float v) { return v * d(rng); };
        lowBoost.setComponents(rnd(lowBoost.L_val), rnd(lowBoost.C_val));
        lowCut.setComponents(rnd(lowCut.L_val), rnd(lowCut.C_val));
        hiBoost.setComponents(rnd(hiBoost.L_val), rnd(hiBoost.C_val));
        hiCut.setComponents(rnd(hiCut.L_val), rnd(hiCut.C_val));
    }
};

// ----- Krane Mybiz (4-band active parametric) -----
class KraneMybizWDF : public WDFCharacterEngine {
public:
    WDFResonantBand b1, b2, b3, b4;
    KraneMybizWDF(float fs)
        : b1(50, 10000, 3.0f, 1.0e-6f, fs), b2(50, 6800, 1.5f, 470e-9f, fs),
          b3(50, 4700, 0.47f, 220e-9f, fs), b4(50, 3300, 0.15f, 100e-9f, fs) {}
    void prepare(float fs) override { b1.prepare(fs); b2.prepare(fs); b3.prepare(fs); b4.prepare(fs); }
    float process(float input) override {
        input = b1.process(input); input = b2.process(input);
        input = b3.process(input); input = b4.process(input);
        return input;
    }
    void randomizeTolerance(float t, int s) override {
        std::mt19937 rng(s); std::uniform_real_distribution<float> d(1.0f - t/100.0f, 1.0f + t/100.0f);
        auto rnd = [&](float v) { return v * d(rng); };
        b1.setComponents(rnd(b1.L_val), rnd(b1.C_val));
        b2.setComponents(rnd(b2.L_val), rnd(b2.C_val));
        b3.setComponents(rnd(b3.L_val), rnd(b3.C_val));
        b4.setComponents(rnd(b4.L_val), rnd(b4.C_val));
    }
};

// ----- West Nugget (3-band tube EQ) -----
class WestNuggetWDF : public WDFCharacterEngine {
public:
    WDFResonantBand shelfLow, bell, shelfHigh;
    WestNuggetWDF(float fs)
        : shelfLow(50, 10000, 4.0f, 2.2e-6f, fs),
          bell(50, 6800, 1.5f, 470e-9f, fs),
          shelfHigh(50, 3300, 0.1f, 100e-9f, fs) {}
    void prepare(float fs) override { shelfLow.prepare(fs); bell.prepare(fs); shelfHigh.prepare(fs); }
    float process(float input) override {
        input = shelfLow.process(input);
        input = bell.process(input);
        input = shelfHigh.process(input);
        return input;
    }
    void randomizeTolerance(float t, int s) override {
        std::mt19937 rng(s); std::uniform_real_distribution<float> d(1.0f - t/100.0f, 1.0f + t/100.0f);
        auto rnd = [&](float v) { return v * d(rng); };
        shelfLow.setComponents(rnd(shelfLow.L_val), rnd(shelfLow.C_val));
        bell.setComponents(rnd(bell.L_val), rnd(bell.C_val));
        shelfHigh.setComponents(rnd(shelfHigh.L_val), rnd(shelfHigh.C_val));
    }
};

// ----- Never 80-8 (4-band Neve-style inductor) -----
class Never80_8WDF : public WDFCharacterEngine {
public:
    WDFResonantBand b1, b2, b3, b4;
    Never80_8WDF(float fs)
        : b1(50, 10000, 6.0f, 3.3e-6f, fs), b2(50, 6800, 2.2f, 1.5e-6f, fs),
          b3(50, 4700, 0.68f, 470e-9f, fs), b4(50, 3300, 0.15f, 100e-9f, fs) {}
    void prepare(float fs) override { b1.prepare(fs); b2.prepare(fs); b3.prepare(fs); b4.prepare(fs); }
    float process(float input) override {
        input = b1.process(input); input = b2.process(input);
        input = b3.process(input); input = b4.process(input);
        return input;
    }
    void randomizeTolerance(float t, int s) override {
        std::mt19937 rng(s); std::uniform_real_distribution<float> d(1.0f - t/100.0f, 1.0f + t/100.0f);
        auto rnd = [&](float v) { return v * d(rng); };
        b1.setComponents(rnd(b1.L_val), rnd(b1.C_val));
        b2.setComponents(rnd(b2.L_val), rnd(b2.C_val));
        b3.setComponents(rnd(b3.L_val), rnd(b3.C_val));
        b4.setComponents(rnd(b4.L_val), rnd(b4.C_val));
    }
};

// ----- Liquid State Solid (4-band SSL-style high-Q) -----
class LiquidStateSolidWDF : public WDFCharacterEngine {
public:
    WDFResonantBand b1, b2, b3, b4;
    LiquidStateSolidWDF(float fs)
        : b1(50, 10000, 2.0f, 470e-9f, fs), b2(50, 6800, 1.0f, 220e-9f, fs),
          b3(50, 4700, 0.47f, 100e-9f, fs), b4(50, 3300, 0.22f, 47e-9f, fs) {}
    void prepare(float fs) override { b1.prepare(fs); b2.prepare(fs); b3.prepare(fs); b4.prepare(fs); }
    float process(float input) override {
        input = b1.process(input); input = b2.process(input);
        input = b3.process(input); input = b4.process(input);
        return input;
    }
    void randomizeTolerance(float t, int s) override {
        std::mt19937 rng(s); std::uniform_real_distribution<float> d(1.0f - t/100.0f, 1.0f + t/100.0f);
        auto rnd = [&](float v) { return v * d(rng); };
        b1.setComponents(rnd(b1.L_val), rnd(b1.C_val));
        b2.setComponents(rnd(b2.L_val), rnd(b2.C_val));
        b3.setComponents(rnd(b3.L_val), rnd(b3.C_val));
        b4.setComponents(rnd(b4.L_val), rnd(b4.C_val));
    }
};

// ================================================================
// Factory
// ================================================================
enum WDFCharType {
    WDF_Off = 0, WDF_MisterPassive, WDF_PoolDake,
    WDF_KraneMybiz, WDF_WestNugget, WDF_Never80_8, WDF_LiquidStateSolid
};

inline std::unique_ptr<WDFCharacterEngine> createWDFEngine(int type, float sr) {
    switch (type) {
        case WDF_MisterPassive:    return std::make_unique<MisterPassiveWDF>(sr);
        case WDF_PoolDake:         return std::make_unique<PoolDakeWDF>(sr);
        case WDF_KraneMybiz:       return std::make_unique<KraneMybizWDF>(sr);
        case WDF_WestNugget:       return std::make_unique<WestNuggetWDF>(sr);
        case WDF_Never80_8:        return std::make_unique<Never80_8WDF>(sr);
        case WDF_LiquidStateSolid: return std::make_unique<LiquidStateSolidWDF>(sr);
        default:                   return nullptr;
    }
}
