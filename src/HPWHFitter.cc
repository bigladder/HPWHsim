
#include <cfloat>

#include "HPWH.hh"
#include "HPWHFitter.hh"

/**	Optimizer for varying model parameters to match metrics, used by
 *  makeGeneric to implement a target UEF. The structure is fairly general, but
 *  currently limited to one metric and two parameters (COP coeffs).
 *  This could be expanded to include other FOMs, such as total energy in 24-h test.
 */

static double targetFunc(void* p0, double& x)
{ // provide single function value
    HPWH::Fitter* fitter = (HPWH::Fitter*)p0;
    auto nParams = fitter->pParams.size();
    auto nMetrics = fitter->pMetrics.size();

    if ((nParams == 1) && (nMetrics == 1))
    {
        auto param = fitter->pParams[0];
        auto metric = fitter->pMetrics[0];

        param->setValue(x);
        metric->eval();
        return metric->currVal;
    }
    return 1.e12;
}

//-----------------------------------------------------------------------------
int HPWH::Fitter::secant( // find x given f(x) (secant method)
    double (*pFunc)(void* pO, double& x),
    // function under investigation; note that it
    //   may CHANGE x re domain limits etc.
    void* pO,               // pointer passed to *pFunc, typ object pointer
    double f,               // f( x) value sought
    double eps,             // convergence tolerance, hi- or both sides
                            //   see also epsLo
    double& x1,             // x 1st guess,
                            //   returned with result
    double& f1,             // f( x1), if known, else pass DBL_MIN
                            //   returned: f( x1), may be != f, if no converge
    double x2,              // x 2nd guess
    double f2 /*=DBL_MIN*/, // f( x2), if known
    double epsLo /*=-1.*/)  // lo-side convergence tolerance

// convergence = f - eps(Lo) <= f1 <= f + eps

// Note: *pFunc MAY have side effects ... on return, f1 = *pFunc( x1)
//      is GUARANTEED to be last call

// returns result code, x1 and f1 always best known solution
//		0: success
//		>0= failed to converge, returned value = # of iterations
//	    <0= 0 slope encountered, returned value = -# of iterations
{
    double fHi = f + eps;
    double fLo = f - (epsLo >= 0. ? epsLo : eps);

    if (f1 == DBL_MIN)
        f1 = (*pFunc)(pO, x1);

    if (f1 <= fHi && f1 >= fLo) // if 1st guess good
        return 0;               //   success: don't do *pFunc( x2)
                                //   (side effects)

    if (f2 == DBL_MIN)
        f2 = (*pFunc)(pO, x2);

    if (fabs(f - f1) > fabs(f - f2)) // make point 1 the closer
    {
        double swap;
        swap = x1;
        x1 = x2;
        x2 = swap;
        swap = f1;
        f1 = f2;
        f2 = swap;
    }

    int i;
    for (i = 0; ++i < 20;) // iterate to refine solution
    {
        if (f1 <= fHi && f1 >= fLo)
        {
            i = 0; // success
            break; //   done; last *pFunc call ...
                   //     1st iteration: *pFunc( x2) + swap
                   //    >1st iteration: *pFunc( x1) below
        }

        if (fabs(f1 - f2) < 1.e-20) // if slope is 0
        {
            i = -i; // tell caller
            break;
        }

        double xN = x1 + (x2 - x1) * (f - f1) / (f2 - f1);
        // secant method: new guess assuming local linearity.
        x2 = x1; // replace older point
        f2 = f1;
        x1 = xN;
        f1 = (*pFunc)(pO, x1); // new value
    }
    return i;
} // ::secant

//-----------------------------------------------------------------------------
///	@brief	Least-squares minimization (one metric, two parameters)
/// @note	see [Numerical Recipes, Ch. 15.5](https://numerical.recipes/book.html)
//-----------------------------------------------------------------------------
void HPWH::Fitter::leastSquares()
{
    bool success = false;
    const int maxIters = 20;

    //
    std::vector<double> dParams;
    for (auto& pParam : pParams)
    {
        dParams.push_back(pParam->dVal);
    }

    auto pMetric = pMetrics[0];
    auto nParams = pParams.size();
    double nu = 0.1;
    for (auto iter = 0; iter < maxIters; ++iter)
    {
        std::string iter_msg = fmt::format("iter {:d}: ", iter);

        pMetric->eval();

        if (pMetric->metricType() == MetricInput::MetricType::UEF)
            iter_msg.append(fmt::format(", UEF: {:g}", pMetric->currVal));

        bool first = true;
        for (std::size_t j = 0; j < nParams; ++j)
        {
            if (!first)
                iter_msg.append(",");

            iter_msg.append(fmt::format(" param{:d}: {:g}", j, pParams[j]->getValue()));
            first = false;
        }

        double dMetric0 = 0.;
        pMetric->evalDiff(dMetric0);
        double FOM0 = dMetric0 * dMetric0;
        double FOM1 = 0., FOM2 = 0.;
        iter_msg.append(fmt::format(", FOM: {:g}", FOM0));
        courier->send_info(iter_msg);

        std::vector<double> paramV(nParams);
        for (std::size_t i = 0; i < nParams; ++i)
        {
            paramV[i] = pParams[i]->getValue();
        }

        std::vector<double> jacobiV(nParams); // 1 x 2
        for (std::size_t j = 0; j < nParams; ++j)
        {
            pParams[j]->setValue(paramV[j] + (pParams[j]->dVal));
            double dMetric;
            pMetric->evalDiff(dMetric);
            jacobiV[j] = (dMetric - dMetric0) / (pParams[j]->dVal);
            pParams[j]->setValue(paramV[j]);
        }

        // try nu
        std::vector<double> invJacobiV1;
        bool got1 = Fitter::Inverter::getLeftDampedInv(nu, jacobiV, invJacobiV1);

        std::vector<double> inc1ParamV(2);
        std::vector<double> paramV1 = paramV;
        if (got1)
        {
            for (std::size_t j = 0; j < nParams; ++j)
            {
                inc1ParamV[j] = -invJacobiV1[j] * dMetric0;
                paramV1[j] += inc1ParamV[j];
                pParams[j]->setValue(paramV1[j]);
            }
            double dMetric;
            pMetric->evalDiff(dMetric);
            FOM1 = dMetric * dMetric;

            // restore
            for (std::size_t j = 0; j < nParams; ++j)
            {
                pParams[j]->setValue(paramV[j]);
            }
        }

        // try nu / 2
        std::vector<double> invJacobiV2;
        bool got2 = Inverter::getLeftDampedInv(nu / 2., jacobiV, invJacobiV2);

        std::vector<double> inc2ParamV(2);
        std::vector<double> paramV2 = paramV;
        if (got2)
        {
            for (std::size_t j = 0; j < nParams; ++j)
            {
                inc2ParamV[j] = -invJacobiV2[j] * dMetric0;
                paramV2[j] += inc2ParamV[j];
                pParams[j]->setValue(paramV2[j]);
            }
            double dMetric;
            pMetric->evalDiff(dMetric);
            FOM2 = dMetric * dMetric;

            // restore
            for (std::size_t j = 0; j < nParams; ++j)
            {
                pParams[j]->setValue(paramV[j]);
            }
        }

        // check for improvement
        if (got1 && got2)
        {
            if ((FOM1 < FOM0) || (FOM2 < FOM0))
            { // at least one improved
                if (FOM1 < FOM2)
                { // pick 1
                    for (std::size_t i = 0; i < nParams; ++i)
                    {
                        pParams[i]->setValue(paramV1[i]);
                        (pParams[i]->dVal) = inc1ParamV[i] / 1.e3;
                        FOM0 = FOM1;
                    }
                }
                else
                { // pick 2
                    for (std::size_t i = 0; i < nParams; ++i)
                    {
                        pParams[i]->setValue(paramV2[i]);
                        (pParams[i]->dVal) = inc2ParamV[i] / 1.e3;
                        FOM0 = FOM2;
                    }
                }
            }
        }
        else
        { // no improvement
            nu *= 10.;
            if (nu > 1.e6)
            {
                send_error("Failure in makeGenericModel using leastSquares");
            }
        }

        if (FOM0 < 1.e-13)
        {
            send_info("Fit converged.");
            success = true;
            break;
        }
    }
    if (!success)
        send_error("Fit did not converge.");
}

void HPWH::Fitter::fit()
{ // minimize the FOM by varying parameters
    auto nParams = pParams.size();
    auto nMetrics = pMetrics.size();

    if ((nParams == 1) && (nMetrics == 1))
    {
        auto param = pParams[0];
        auto metric = pMetrics[0];

        double val0 = param->getValue();
        metric->eval();
        double f0 = metric->currVal;

        double val1 = (1.001) * val0;
        param->setValue(val1);
        metric->eval();
        double f1 = metric->currVal;

        param->setValue(val0);

        int iters = secant(targetFunc, this, metric->targetVal, 1.e-12, val0, f0, val1, f1, 1.e-12);
        if (iters < 0)
            send_error("Failure in makeGenericModel using secant");
    }
    else if ((nParams == 2) && (nMetrics == 1))
    {
        leastSquares();
    }
    else
    {
        send_error("Cannot perform fit.");
    }
}
