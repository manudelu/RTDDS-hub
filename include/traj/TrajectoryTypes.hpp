#pragma once

struct Cubic
{
    double a0_, a1_, a2_, a3_;

    Cubic() = default;

    Cubic(double q0, double qf, double T,
          double qd0 = 0.0, double qdf = 0.0)
        : a0_{q0}
        , a1_{qd0}
    {
        a2_ = ( 3.0*(qf - q0) / (T*T)) - (2.0*qd0 / T) - (qdf / T);
        a3_ = (-2.0*(qf - q0) / (T*T*T)) + ((qd0 + qdf) / (T*T));
    }

    double position(double t) const
    {
        return a0_ + a1_*t + a2_*t*t + a3_*t*t*t;
    }
    double velocity(double t) const
    {
        return a1_ + 2.0*a2_*t + 3.0*a3_*t*t;
    }
};

struct Quintic
{
    double a0_, a1_, a2_, a3_, a4_, a5_;

    Quintic() = default;

    Quintic(double q0, double qf, double T,
            double qd0 = 0.0, double qdf = 0.0,
            double qdd0 = 0.0, double qddf = 0.0)
        : a0_{q0}
        , a1_{qd0}
        , a2_{qdd0 / 2.0}
    {
        const double T2 = T*T, T3 = T2*T, T4 = T3*T, T5 = T4*T;
        const double dq = qf - q0;

        a3_ = (20.0*dq - (8.0*qdf + 12.0*qd0)*T - (3.0*qdd0 - qddf)*T2) / (2.0*T3);
        a4_ = (30.0*q0 - 30.0*qf + (14.0*qdf + 16.0*qd0)*T + (3.0*qdd0 - 2.0*qddf)*T2) / (2.0*T4);
        a5_ = (12.0*dq - (6.0*qdf + 6.0*qd0)*T - (qdd0 - qddf)*T2) / (2.0*T5);
    }

    double position(double t) const
    {
        const double t2 = t*t, t3 = t2*t, t4 = t3*t, t5 = t4*t;
        return a0_ + a1_*t + a2_*t2 + a3_*t3 + a4_*t4 + a5_*t5;
    }
    double velocity(double t) const
    {
        const double t2 = t*t, t3 = t2*t, t4 = t3*t;
        return a1_ + 2.0*a2_*t + 3.0*a3_*t2 + 4.0*a4_*t3 + 5.0*a5_*t4;
    }
};
