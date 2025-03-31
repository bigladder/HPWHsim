
#include <cfloat>

#include "HPWH.hh"
#include "HPWHFitter.hh"

// evaluate the current metric using provided single-parameter value
double HPWH::Fitter::getMetricSingleParameter(double x)
{
    auto nParameters = parameters.size();
    auto nMetrics = metrics.size();

    if ((nParameters == 1) && (nMetrics == 1))
    {
        *parameters[0]->data_ptr = x;
        return metrics[0]->evaluate();
    }
    return 1.e12;
}

//-----------------------------------------------------------------------------
///	@brief	target function used by secant
//-----------------------------------------------------------------------------
static double targetFunc(void* p0, double& x)
{ // provide single function value
    auto fitter = static_cast<HPWH::Fitter*>(p0);
    return fitter->getMetricSingleParameter(x);
}

//-----------------------------------------------------------------------------
///	@brief	secant function, directly from cse/nummeth.cpp
//-----------------------------------------------------------------------------
static int secant( // find x given f(x) (secant method)
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
///	@brief	Left pseudo-inverse an m x n (1 x 2) matrix, J, (one metric, two parameters)
///         with scaling of diagonal terms (damping off-diagonal terms of inverse)
///         J^L = (J^T * J + nu * diag(J^T * J))^-1 * J^T
//-----------------------------------------------------------------------------
static bool getLeftDampedInverse(const double nu,
                                 const std::vector<double>& matV, // 1 x 2
                                 std::vector<double>& invMatV     // 2 x 1
)
{
    constexpr double thresh = 1.e-12;

    if (matV.size() != 2)
    {
        return false;
    }

    double a = matV[0];
    double b = matV[1];

    double A = (1. + nu) * a * a;
    double B = (1. + nu) * b * b;
    double C = a * b;
    double det = A * B - C * C;

    if (fabs(det) < thresh)
    {
        return false;
    }

    invMatV.resize(2);
    invMatV[0] = (a * B - b * C) / det;
    invMatV[1] = (-a * C + b * A) / det;
    return true;
}

//-----------------------------------------------------------------------------
///	@brief	Least-squares minimization (one metric, two parameters)
/// @note	see [Numerical Recipes, Ch. 15.5](https://numerical.recipes/book.html)
///         This can be generalized with suitable matrix utilities.
//-----------------------------------------------------------------------------
void HPWH::Fitter::performLeastSquaresMinimization()
{
    bool success = false;
    const int maxIters = 20;

    auto metric = metrics[0]; // currently, one metric onlu
    auto nParameters = parameters.size();
    double nu = 0.001; // damping term
    for (auto iter = 0; iter < maxIters; ++iter)
    {
        std::string iter_msg = fmt::format("iter {:d}: ", iter);

        double error = metric->findError();
        double FOM = error * error; // figure of merit

        iter_msg.append(fmt::format(", FOM: {:g}", FOM));
        // courier->send_info(iter_msg);

        std::vector<double> parameterV(nParameters);
        for (std::size_t i = 0; i < nParameters; ++i)
        {
            parameterV[i] = *parameters[i]->data_ptr;
        }

        std::vector<double> jacobiV(nParameters); // 1 x 2
        for (std::size_t i = 0; i < nParameters; ++i)
        { // get jacobian
            *parameters[i]->data_ptr = parameterV[i] + (parameters[i]->increment);
            jacobiV[i] = (metric->findError() - error) / (parameters[i]->increment);
            *parameters[i]->data_ptr = parameterV[i]; // restore
        }

        bool improved = false;
        std::vector<double> inverseJacobiV; // invert jacobian using damping nu
        if (getLeftDampedInverse(nu, jacobiV, inverseJacobiV))
        {
            // find error using incremented parameters
            std::vector<double> incrementParamV(2);
            for (std::size_t i = 0; i < nParameters; ++i)
            {
                incrementParamV[i] = -inverseJacobiV[i] * error;
                *parameters[i]->data_ptr = parameterV[i] + incrementParamV[i]; // increment
            }
            double incrementedError = metric->findError();
            double incrementedFOM =
                incrementedError * incrementedError; // FOM with increments applied

            // restore parameters
            for (std::size_t j = 0; j < nParameters; ++j)
            {
                *parameters[j]->data_ptr = parameterV[j];
            }

            if (incrementedFOM < FOM)
            {
                // improved, so apply increments and reduce nu
                for (std::size_t i = 0; i < nParameters; ++i)
                {
                    *parameters[i]->data_ptr = parameterV[i] + incrementParamV[i];
                    (parameters[i]->increment) = fabs(incrementParamV[i]) / 1.e3;
                    FOM = incrementedFOM;
                }
                nu /= 10.;
                improved = true;
            }
        }

        // no improvement, increase nu or fail
        if (!improved)
        {
            nu *= 10.;
            if (nu > 1.e6)
            {
                send_error("Failure in performLeastSquaresMinimization");
            }
        }

        if (FOM < 1.e-12)
        {
            // send_info("Fit converged.");
            success = true;
            break;
        }
    }
    if (!success)
        send_error("Fit did not converge.");
}

//-----------------------------------------------------------------------------
///	@brief	Perform fit: vary parameters to meet metrics
//-----------------------------------------------------------------------------
void HPWH::Fitter::fit()
{
    auto nParameters = parameters.size();
    auto nMetrics = metrics.size();

    if ((nParameters == 1) && (nMetrics == 1))
    { // use secant
        auto parameter = parameters[0];
        auto metric = metrics[0];

        double value0 = *parameter->data_ptr;
        double f0 = metric->evaluate();

        double value1 = value0 + parameter->increment;
        *parameter->data_ptr = value1;
        double f1 = metric->evaluate();

        *parameter->data_ptr = value0;

        int iters =
            secant(targetFunc, this, metric->targetValue, 1.e-12, value0, f0, value1, f1, 1.e-12);
        if (iters < 0)
            send_error("Failure in makeGenericModel using secant");
    }
    else if ((nParameters == 2) && (nMetrics == 1))
    { // use least-squares
        performLeastSquaresMinimization();
    }
    else
    {
        send_error("Cannot perform fit.");
    }
}
