#pragma once
#include <cmath>

// ============================================================
// Clean-room WDF implementation
// Wave Digital Filters — Fettweis (1970s), public domain math
// ============================================================

namespace wdf {

struct Members {
    double R = 1e-9;
    double G = 1.0 / R;
    double a = 0.0, b = 0.0;
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
    virtual void incident(double x) = 0;
    virtual double reflected() = 0;
    double portVoltage() const { return (w.a + w.b) * 0.5; }
    double portCurrent() const { return (w.a - w.b) * 0.5 * w.G; }
};

// ============ 1-Port Elements ============

class Resistor : public Base {
public:
    double R_val;
    explicit Resistor(double r) : R_val(r) { calcImpedance(); }
    void setValue(double r) { if (r != R_val) { R_val = r; propagateImpedance(); } }
    void calcImpedance() override { w.R = R_val; w.G = 1.0 / w.R; }
    void incident(double x) override { w.a = x; }
    double reflected() override { w.b = 0.0; return w.b; }
};

class Capacitor : public Base {
public:
    double C_val, fs;
    double z = 0.0;
    explicit Capacitor(double c, double sr) : C_val(c), fs(sr) { calcImpedance(); }
    void setValue(double c) { if (c != C_val) { C_val = c; propagateImpedance(); } }
    void prepare(double sr) { fs = sr; propagateImpedance(); z = 0.0; }
    void calcImpedance() override { w.R = 1.0 / (2.0 * C_val * fs); w.G = 1.0 / w.R; }
    void incident(double x) override { w.a = x; z = w.a; }
    double reflected() override { w.b = z; return w.b; }
};

class Inductor : public Base {
public:
    double L_val, fs;
    double z = 0.0;
    explicit Inductor(double l, double sr) : L_val(l), fs(sr) { calcImpedance(); }
    void setValue(double l) { if (l != L_val) { L_val = l; propagateImpedance(); } }
    void prepare(double sr) { fs = sr; propagateImpedance(); z = 0.0; }
    void calcImpedance() override { w.R = 2.0 * L_val * fs; w.G = 1.0 / w.R; }
    void incident(double x) override { w.a = x; z = w.a; }
    double reflected() override { w.b = -z; return w.b; }
};

// ============ Adaptors ============

class SeriesAdaptor : public Base {
public:
    Base& p1; Base& p2;
    double port1Reflect = 1.0;
    SeriesAdaptor(Base& a, Base& b) : p1(a), p2(b) {
        p1.parent = this; p2.parent = this; calcImpedance();
    }
    void calcImpedance() override {
        w.R = p1.w.R + p2.w.R; w.G = 1.0 / w.R;
        port1Reflect = p1.w.R / w.R;
    }
    void incident(double x) override {
        w.a = x;
        double b1 = p1.w.b - port1Reflect * (x + p1.w.b + p2.w.b);
        p1.incident(b1);
        p2.incident(-(x + b1));
    }
    double reflected() override {
        w.b = -(p1.reflected() + p2.reflected());
        return w.b;
    }
};

class ParallelAdaptor : public Base {
public:
    Base& p1; Base& p2;
    double port1Reflect = 1.0;
    double bDiff = 0.0;
    ParallelAdaptor(Base& a, Base& b) : p1(a), p2(b) {
        p1.parent = this; p2.parent = this; calcImpedance();
    }
    void calcImpedance() override {
        w.G = p1.w.G + p2.w.G; w.R = 1.0 / w.G;
        port1Reflect = p1.w.G / w.G;
    }
    void incident(double x) override {
        double b2 = w.b - p2.w.b + x;
        p1.incident(b2 + bDiff);
        p2.incident(b2);
        w.a = x;
    }
    double reflected() override {
        p1.reflected(); p2.reflected();
        bDiff = p2.w.b - p1.w.b;
        w.b = p2.w.b - port1Reflect * bDiff;
        return w.b;
    }
};

// ============ Source ============

class IdealVoltageSource : public Base {
public:
    double Vs = 0.0;
    explicit IdealVoltageSource(Base& next) { next.parent = this; calcImpedance(); }
    void calcImpedance() override { w.R = 0.0; w.G = 1e15; }
    void setVoltage(double v) { Vs = v; }
    void incident(double x) override { w.a = x; }
    double reflected() override { w.b = -w.a + 2.0 * Vs; return w.b; }
};

// ============ Float convenience types ============

class ResistorF : public Resistor { public: explicit ResistorF(float r) : Resistor(r) {} void setValue(float r) { Resistor::setValue(r); } };
class CapacitorF : public Capacitor { public: explicit CapacitorF(float c, float sr) : Capacitor(c, sr) {} void setValue(float c) { Capacitor::setValue(c); } void prepare(float sr) { Capacitor::prepare(sr); } };
class InductorF : public Inductor { public: explicit InductorF(float l, float sr) : Inductor(l, sr) {} void setValue(float l) { Inductor::setValue(l); } void prepare(float sr) { Inductor::prepare(sr); } };
class SeriesAdaptorF : public SeriesAdaptor { public: SeriesAdaptorF(Base& a, Base& b) : SeriesAdaptor(a, b) {} };
class ParallelAdaptorF : public ParallelAdaptor { public: ParallelAdaptorF(Base& a, Base& b) : ParallelAdaptor(a, b) {} };
class IdealVoltageSourceF : public IdealVoltageSource { public: IdealVoltageSourceF(Base& n) : IdealVoltageSource(n) {} void setVoltage(float v) { IdealVoltageSource::setVoltage(v); } };

} // namespace wdf
