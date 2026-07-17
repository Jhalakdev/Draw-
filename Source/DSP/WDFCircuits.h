#pragma once
#include "WDFCore.h"
#include <random>
#include <memory>
#include <vector>

// ================================================================
// Base engine
// ================================================================
class WDFCharacterEngine {
public:
    virtual ~WDFCharacterEngine() = default;
    virtual void prepare(float sampleRate) = 0;
    virtual float process(float input) = 0;
    virtual void randomizeTolerance(float tolPercent, int seed) = 0;
};

// ================================================================
// WDF Parallel EQ — Vin -> Rsrc -> (all R||L||C banks in parallel) -> GND
// Output = voltage across the parallel bank
// Each bank creates a subtle notch at its resonant frequency
// ================================================================
class WDFParallelEQ : public WDFCharacterEngine {
public:
    struct Band {
        wdf::Resistor rDamp;
        wdf::Inductor inductor;
        wdf::Capacitor capacitor;
        wdf::ParallelAdaptor lcParallel;
        wdf::ParallelAdaptor rlcParallel;

        float L_val, C_val, R_val;

        Band(float r, float l, float c, float fs)
            : rDamp(r), inductor(l, fs), capacitor(c, fs),
              lcParallel(inductor, capacitor),
              rlcParallel(rDamp, lcParallel),
              L_val(l), C_val(c), R_val(r) {}

        void prepare(float fs) {
            inductor.prepare(fs);
            capacitor.prepare(fs);
        }

        void randomize(std::mt19937& rng) {
            std::uniform_real_distribution<float> d(0.95f, 1.05f);
            float l = L_val * d(rng);
            float c = C_val * d(rng);
            inductor.setValue(l);
            capacitor.setValue(c);
            L_val = l; C_val = c;
        }
    };

    Band b1, b2, b3, b4;
    wdf::ParallelAdaptor p34, p234, allBands;
    wdf::Resistor rSrc;
    wdf::SeriesAdaptor full;
    wdf::IdealVoltageSource vin;

    WDFParallelEQ(float rs, float r1, float l1, float c1,
                  float r2, float l2, float c2,
                  float r3, float l3, float c3,
                  float r4, float l4, float c4, float fs)
        : b1(r1, l1, c1, fs), b2(r2, l2, c2, fs),
          b3(r3, l3, c3, fs), b4(r4, l4, c4, fs),
          p34(b3.rlcParallel, b4.rlcParallel),
          p234(b2.rlcParallel, p34),
          allBands(b1.rlcParallel, p234),
          rSrc(rs), full(rSrc, allBands), vin(full) {}

    void prepare(float fs) override {
        b1.prepare(fs); b2.prepare(fs);
        b3.prepare(fs); b4.prepare(fs);
    }

    float process(float input) override {
        vin.setVoltage(input);
        vin.incident(full.reflected());
        full.incident(vin.reflected());
        return allBands.portVoltage();
    }

    void randomizeTolerance(float t, int s) override {
        std::mt19937 rng(s);
        b1.randomize(rng); b2.randomize(rng);
        b3.randomize(rng); b4.randomize(rng);
    }
};

// ================================================================
// 3-band version for West Nugget
// ================================================================
class WDFParallelEQ3 : public WDFCharacterEngine {
public:
    struct Band {
        wdf::Resistor rDamp;
        wdf::Inductor inductor;
        wdf::Capacitor capacitor;
        wdf::ParallelAdaptor lcParallel;
        wdf::ParallelAdaptor rlcParallel;

        float L_val, C_val, R_val;

        Band(float r, float l, float c, float fs)
            : rDamp(r), inductor(l, fs), capacitor(c, fs),
              lcParallel(inductor, capacitor),
              rlcParallel(rDamp, lcParallel),
              L_val(l), C_val(c), R_val(r) {}

        void prepare(float fs) {
            inductor.prepare(fs);
            capacitor.prepare(fs);
        }

        void randomize(std::mt19937& rng) {
            std::uniform_real_distribution<float> d(0.95f, 1.05f);
            float l = L_val * d(rng);
            float c = C_val * d(rng);
            inductor.setValue(l);
            capacitor.setValue(c);
            L_val = l; C_val = c;
        }
    };

    Band b1, b2, b3;
    wdf::ParallelAdaptor p23, allBands;
    wdf::Resistor rSrc;
    wdf::SeriesAdaptor full;
    wdf::IdealVoltageSource vin;

    WDFParallelEQ3(float rs, float r1, float l1, float c1,
                   float r2, float l2, float c2,
                   float r3, float l3, float c3, float fs)
        : b1(r1, l1, c1, fs), b2(r2, l2, c2, fs), b3(r3, l3, c3, fs),
          p23(b2.rlcParallel, b3.rlcParallel),
          allBands(b1.rlcParallel, p23),
          rSrc(rs), full(rSrc, allBands), vin(full) {}

    void prepare(float fs) override {
        b1.prepare(fs); b2.prepare(fs); b3.prepare(fs);
    }

    float process(float input) override {
        vin.setVoltage(input);
        vin.incident(full.reflected());
        full.incident(vin.reflected());
        return allBands.portVoltage();
    }

    void randomizeTolerance(float t, int s) override {
        std::mt19937 rng(s);
        b1.randomize(rng); b2.randomize(rng); b3.randomize(rng);
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
        case WDF_MisterPassive:
            return std::make_unique<WDFParallelEQ>(
                47.0f,
                10000.0f, 8.0f, 2.2e-6f,
                6800.0f,  1.5f, 1.0e-6f,
                4700.0f,  0.5f, 470e-9f,
                3300.0f,  0.1f, 100e-9f, sr);

        case WDF_PoolDake:
            return std::make_unique<WDFParallelEQ>(
                47.0f,
                10000.0f, 8.0f, 2.2e-6f,
                6800.0f,  1.5f, 1.0e-6f,
                4700.0f,  0.5f, 470e-9f,
                3300.0f,  0.1f, 100e-9f, sr);

        case WDF_KraneMybiz:
            return std::make_unique<WDFParallelEQ>(
                47.0f,
                8200.0f,  3.0f, 1.0e-6f,
                5600.0f,  1.5f, 470e-9f,
                3900.0f,  0.47f, 220e-9f,
                2700.0f,  0.15f, 100e-9f, sr);

        case WDF_WestNugget:
            return std::make_unique<WDFParallelEQ3>(
                47.0f,
                8200.0f,  4.0f, 2.2e-6f,
                5600.0f,  1.5f, 470e-9f,
                2700.0f,  0.1f, 100e-9f, sr);

        case WDF_Never80_8:
            return std::make_unique<WDFParallelEQ>(
                47.0f,
                10000.0f, 6.0f, 3.3e-6f,
                6800.0f,  2.2f, 1.5e-6f,
                4700.0f,  0.68f, 470e-9f,
                3300.0f,  0.15f, 100e-9f, sr);

        case WDF_LiquidStateSolid:
            return std::make_unique<WDFParallelEQ>(
                47.0f,
                10000.0f, 2.0f, 470e-9f,
                6800.0f,  1.0f, 220e-9f,
                4700.0f,  0.47f, 100e-9f,
                3300.0f,  0.22f, 47e-9f, sr);

        default: return nullptr;
    }
}
