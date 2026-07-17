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
    wdf::ResistorF rSource, rDamp;
    wdf::InductorF inductor;
    wdf::CapacitorF capacitor;
    wdf::ParallelAdaptorF lcParallel;
    wdf::ParallelAdaptorF rlcParallel;
    wdf::SeriesAdaptorF fullSeries;
    wdf::IdealVoltageSourceF vin;

    double L_val, C_val;
    double fs_val;

    WDFResonantBand(double rSrc, double rDampVal, double lVal, double cVal, double fs)
        : rSource(rSrc), rDamp(rDampVal), inductor(lVal, fs), capacitor(cVal, fs),
          lcParallel(inductor, capacitor), rlcParallel(rDamp, lcParallel),
          fullSeries(rSource, rlcParallel), vin(fullSeries),
          L_val(lVal), C_val(cVal), fs_val(fs) {}

    void prepare(double fs) {
        fs_val = fs;
        inductor.prepare(fs);
        capacitor.prepare(fs);
    }

    float process(float input) {
        vin.setVoltage(input);
        vin.incident(fullSeries.reflected());
        fullSeries.incident(vin.reflected());
        return (float)rlcParallel.portVoltage();
    }

    void setFreq(double f) {
        double w = 6.283185307179586 * f;
        if (w > 1e-3 && L_val > 1e-12) {
            C_val = 1.0 / (L_val * w * w);
            capacitor.setValue(C_val);
        }
    }

    void setQ(double q) {
        if (q < 0.1) q = 0.1;
        double w0 = getFreq() * 6.283185307179586;
        if (w0 > 1e-3) rDamp.setValue(w0 * L_val * q);
    }

    void setComponents(double l, double c) {
        L_val = l; C_val = c;
        inductor.setValue(l); capacitor.setValue(c);
    }

    void setRs(double r) { rSource.setValue(r); }

    double getFreq() const {
        if (L_val < 1e-12 || C_val < 1e-12) return 0.0;
        return 1.0 / (6.283185307179586 * std::sqrt(L_val * C_val));
    }
};

// ================================================================
// Base engine + all character WDF engines
// ================================================================
class WDFCharacterEngine {
public:
    virtual ~WDFCharacterEngine() = default;
    virtual void prepare(double sampleRate) = 0;
    virtual float process(float input) = 0;
    virtual void randomizeTolerance(double tolPercent, int seed) = 0;
};

// ----- Mister Passive (4-band LC) -----
class MisterPassiveWDF : public WDFCharacterEngine {
public:
    WDFResonantBand band1, band2, band3, band4;
    MisterPassiveWDF(double fs)
        : band1(600, 1000, 8.0, 2.2e-6, fs),
          band2(600, 470, 1.5, 1.0e-6, fs),
          band3(600, 330, 0.5, 470e-9, fs),
          band4(600, 220, 0.1, 100e-9, fs) {}
    void prepare(double fs) override { band1.prepare(fs); band2.prepare(fs); band3.prepare(fs); band4.prepare(fs); }
    float process(float input) override {
        input = band1.process(input); input = band2.process(input);
        input = band3.process(input); input = band4.process(input);
        return input;
    }
    void randomizeTolerance(double t, int s) override {
        std::mt19937 rng(s); std::uniform_real_distribution<double> d(1.0 - t/100.0, 1.0 + t/100.0);
        auto rnd = [&](double v) { return v * d(rng); };
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
    PoolDakeWDF(double fs)
        : lowBoost(600, 1000, 8.0, 2.2e-6, fs), lowCut(600, 470, 1.5, 1.0e-6, fs),
          hiBoost(600, 330, 0.5, 470e-9, fs), hiCut(600, 220, 0.1, 100e-9, fs) {}
    void prepare(double fs) override { lowBoost.prepare(fs); lowCut.prepare(fs); hiBoost.prepare(fs); hiCut.prepare(fs); }
    float process(float input) override {
        input = lowBoost.process(input); input = lowCut.process(input);
        input = hiBoost.process(input); input = hiCut.process(input);
        return input;
    }
    void randomizeTolerance(double t, int s) override {
        std::mt19937 rng(s); std::uniform_real_distribution<double> d(1.0 - t/100.0, 1.0 + t/100.0);
        auto rnd = [&](double v) { return v * d(rng); };
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
    KraneMybizWDF(double fs)
        : b1(600, 1800, 3.0, 1.0e-6, fs), b2(600, 1200, 1.5, 470e-9, fs),
          b3(600, 820, 0.47, 220e-9, fs), b4(600, 560, 0.15, 100e-9, fs) {}
    void prepare(double fs) override { b1.prepare(fs); b2.prepare(fs); b3.prepare(fs); b4.prepare(fs); }
    float process(float input) override {
        input = b1.process(input); input = b2.process(input);
        input = b3.process(input); input = b4.process(input);
        return input;
    }
    void randomizeTolerance(double t, int s) override {
        std::mt19937 rng(s); std::uniform_real_distribution<double> d(1.0 - t/100.0, 1.0 + t/100.0);
        auto rnd = [&](double v) { return v * d(rng); };
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
    WestNuggetWDF(double fs)
        : shelfLow(1200, 1000, 4.0, 2.2e-6, fs),
          bell(600, 470, 1.5, 470e-9, fs),
          shelfHigh(1200, 560, 0.1, 100e-9, fs) {}
    void prepare(double fs) override { shelfLow.prepare(fs); bell.prepare(fs); shelfHigh.prepare(fs); }
    float process(float input) override {
        input = shelfLow.process(input);
        input = bell.process(input);
        input = shelfHigh.process(input);
        return input;
    }
    void randomizeTolerance(double t, int s) override {
        std::mt19937 rng(s); std::uniform_real_distribution<double> d(1.0 - t/100.0, 1.0 + t/100.0);
        auto rnd = [&](double v) { return v * d(rng); };
        shelfLow.setComponents(rnd(shelfLow.L_val), rnd(shelfLow.C_val));
        bell.setComponents(rnd(bell.L_val), rnd(bell.C_val));
        shelfHigh.setComponents(rnd(shelfHigh.L_val), rnd(shelfHigh.C_val));
    }
};

// ----- Never 80-8 (4-band Neve-style inductor) -----
class Never80_8WDF : public WDFCharacterEngine {
public:
    WDFResonantBand b1, b2, b3, b4;
    Never80_8WDF(double fs)
        : b1(600, 1000, 6.0, 3.3e-6, fs), b2(600, 680, 2.2, 1.5e-6, fs),
          b3(600, 470, 0.68, 470e-9, fs), b4(600, 330, 0.15, 100e-9, fs) {}
    void prepare(double fs) override { b1.prepare(fs); b2.prepare(fs); b3.prepare(fs); b4.prepare(fs); }
    float process(float input) override {
        input = b1.process(input); input = b2.process(input);
        input = b3.process(input); input = b4.process(input);
        return input;
    }
    void randomizeTolerance(double t, int s) override {
        std::mt19937 rng(s); std::uniform_real_distribution<double> d(1.0 - t/100.0, 1.0 + t/100.0);
        auto rnd = [&](double v) { return v * d(rng); };
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
    LiquidStateSolidWDF(double fs)
        : b1(600, 2200, 2.0, 470e-9, fs), b2(600, 1800, 1.0, 220e-9, fs),
          b3(600, 1200, 0.47, 100e-9, fs), b4(600, 820, 0.22, 47e-9, fs) {}
    void prepare(double fs) override { b1.prepare(fs); b2.prepare(fs); b3.prepare(fs); b4.prepare(fs); }
    float process(float input) override {
        input = b1.process(input); input = b2.process(input);
        input = b3.process(input); input = b4.process(input);
        return input;
    }
    void randomizeTolerance(double t, int s) override {
        std::mt19937 rng(s); std::uniform_real_distribution<double> d(1.0 - t/100.0, 1.0 + t/100.0);
        auto rnd = [&](double v) { return v * d(rng); };
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

inline std::unique_ptr<WDFCharacterEngine> createWDFEngine(int type, double sr) {
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
