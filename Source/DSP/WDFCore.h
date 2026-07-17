#pragma once
#include <cmath>

namespace wdf {

struct Members {
    float R = 1e-9f;
    float G = 1.0f / R;
    float a = 0.0f, b = 0.0f;
};

class Base {
public:
    Base* parent = nullptr;
    Members w;
    virtual ~Base() = default;
    virtual void calcImpedance() = 0;
    virtual void propagateImpedance() {
        calcImpedance();
        if (parent) parent->propagateImpedance();
    }
    virtual void incident(float x) = 0;
    virtual float reflected() = 0;
    float portVoltage() const { return (w.a + w.b) * 0.5f; }
};

class Resistor : public Base {
public:
    float R_val;
    explicit Resistor(float r) : R_val(r) { calcImpedance(); }
    void setValue(float r) { if (r != R_val) { R_val = r; propagateImpedance(); } }
    void calcImpedance() override { w.R = R_val; w.G = 1.0f / w.R; }
    void incident(float x) override { w.a = x; }
    float reflected() override { w.b = 0.0f; return w.b; }
};

class Inductor : public Base {
public:
    float L_val, fs;
    float z = 0.0f;
    static constexpr float dcLeak = 0.9999995f;
    explicit Inductor(float l, float sr) : L_val(l), fs(sr) { calcImpedance(); }
    void setValue(float l) { if (l != L_val) { L_val = l; propagateImpedance(); } }
    void prepare(float sr) { fs = sr; propagateImpedance(); z = 0.0f; }
    void calcImpedance() override { w.R = 2.0f * L_val * fs; w.G = 1.0f / w.R; }
    void incident(float x) override { w.a = x; z = w.a; }
    float reflected() override { w.b = -z; z *= dcLeak; return w.b; }
};

class Capacitor : public Base {
public:
    float C_val, fs;
    float z = 0.0f;
    static constexpr float dcLeak = 0.9999995f;
    explicit Capacitor(float c, float sr) : C_val(c), fs(sr) { calcImpedance(); }
    void setValue(float c) { if (c != C_val) { C_val = c; propagateImpedance(); } }
    void prepare(float sr) { fs = sr; propagateImpedance(); z = 0.0f; }
    void calcImpedance() override { w.R = 1.0f / (2.0f * C_val * fs); w.G = 1.0f / w.R; }
    void incident(float x) override { w.a = x; z = w.a; }
    float reflected() override { w.b = z; z *= dcLeak; return w.b; }
};

class SeriesAdaptor : public Base {
public:
    Base& p1; Base& p2;
    float port1Reflect = 1.0f;
    SeriesAdaptor(Base& a, Base& b) : p1(a), p2(b) {
        p1.parent = this; p2.parent = this; calcImpedance();
    }
    void calcImpedance() override {
        w.R = p1.w.R + p2.w.R; w.G = 1.0f / w.R;
        port1Reflect = p1.w.R / w.R;
    }
    void incident(float x) override {
        w.a = x;
        float b1 = p1.w.b - port1Reflect * (x + p1.w.b + p2.w.b);
        p1.incident(b1);
        p2.incident(-(x + b1));
    }
    float reflected() override {
        w.b = -(p1.reflected() + p2.reflected());
        return w.b;
    }
};

class ParallelAdaptor : public Base {
public:
    Base& p1; Base& p2;
    float port1Reflect = 1.0f;
    float bDiff = 0.0f;
    ParallelAdaptor(Base& a, Base& b) : p1(a), p2(b) {
        p1.parent = this; p2.parent = this; calcImpedance();
    }
    void calcImpedance() override {
        w.G = p1.w.G + p2.w.G; w.R = 1.0f / w.G;
        port1Reflect = p1.w.G / w.G;
    }
    void incident(float x) override {
        float b2 = w.b - p2.w.b + x;
        p1.incident(b2 + bDiff);
        p2.incident(b2);
        w.a = x;
    }
    float reflected() override {
        p1.reflected(); p2.reflected();
        bDiff = p2.w.b - p1.w.b;
        w.b = p2.w.b - port1Reflect * bDiff;
        return w.b;
    }
};

class IdealVoltageSource : public Base {
public:
    float Vs = 0.0f;
    explicit IdealVoltageSource(Base& next) { next.parent = this; calcImpedance(); }
    void calcImpedance() override { w.R = 0.0f; w.G = 1e15f; }
    void setVoltage(float v) { Vs = v; }
    void incident(float x) override { w.a = x; }
    float reflected() override { w.b = -w.a + 2.0f * Vs; return w.b; }
};

} // namespace wdf
