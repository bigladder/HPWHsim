#ifndef HPWHFITTER_hh
#define HPWHFITTER_hh

#include <cfloat>

#include "HPWH.hh"

/**	Optimizer for varying model parameters to match metrics, used by
 *  make_generic to implement a target UEF. The structure is fairly general, but
 *  currently limited to one figure-of-merit (UEF) and two parameters (COP coeffs).
 *  This could be expanded to include other FOMs, such as total energy in 24-h test.
 */
struct HPWH::Fitter
{
    struct ParamInfo
    { // base class for parameter information

        virtual void assign(double*& val) = 0;
        virtual void showInfo(std::ostream& os) = 0;
    };

    struct COP_CoefInfo : public ParamInfo,
                          COP_CoefInput
    { // COP coef parameter info
        HPWH* hpwh;

        COP_CoefInfo(COP_CoefInput cop_CoedInput, HPWH* hpwh_in = nullptr)
            : ParamInfo(), COP_CoefInput(cop_CoedInput), hpwh(hpwh_in)
        {
        }

        void assign(double*& val) override
        { // get a reference to the HPWH member variable
            val = nullptr;
            HPWH::HeatSource* heatSource;

            hpwh->getNthHeatSource(hpwh->compressorIndex, heatSource);

            auto& perfMap = heatSource->perfMap;
            if (tempIndex >= perfMap.size())
            {
                hpwh->send_error("Invalid heat-source performance-map temperature index.");
            }

            auto& perfPoint = perfMap[tempIndex];
            auto& copCoeffs = perfPoint.COP_coeffs;
            if (power >= copCoeffs.size())
            {
                hpwh->send_error("Invalid heat-source performance-map cop-coefficient power.");
            }
            val = &copCoeffs[power];
        };

        void showInfo(std::ostream& os) override
        {
            os << ", temperature index = " << tempIndex;
            os << ", power = " << power;
        }
    };

    struct Param
    { // base class for parameter
        double* val;
        double dVal;
        bool hold = false;

        Param() : val(nullptr), dVal(1.e3) {}

        virtual void assignVal() = 0;
        virtual void show(std::ostream& os) = 0;
        virtual ~Param() = default;

        virtual ParamInput::ParamType paramType() { return ParamInput::ParamType::none; }
    };

    struct COP_Coef : public Param,
                      COP_CoefInfo
    { // COP coefficient parameter
        COP_Coef(COP_CoefInput& copCoefInput, HPWH* hpwh)
            : Param(), COP_CoefInfo(copCoefInput, hpwh)
        {
            dVal = 1.e-9;
        }

        void assignVal() override { assign(val); }

        ParamInput::ParamType paramType() override { return ParamInput::ParamType::PerfCoef; }

        void show(std::ostream& os) override
        {
            showInfo(os);
            os << ": " << *val << "\n";
        }
    };

    struct Merit
    { // base class for a figure of merit
        double currVal;
        double targetVal;
        double tolVal; // tolerance

        Merit() : currVal(0.), targetVal(0.), tolVal(1.e-6) {}

        virtual void eval() = 0;
        virtual void evalDiff(double& diff) = 0;

        virtual MeritInput::MeritType meritType() { return MeritInput::MeritType::none; }
    };

    struct UEF_Merit : public Merit
    { // UEF as a figure of merit
        HPWH* hpwh;
        FirstHourRating* firstHourRating;
        StandardTestOptions* standardTestOptions;
        double ambientT_C = 19.7; // EERE-2019-BT-TP-0032-0058, p. 40435

        UEF_Merit(UEF_MeritInput& uefMeritInput,
                  FirstHourRating* firstHourRating_in,
                  StandardTestOptions* standardTestOptions_in,
                  HPWH* hpwh_in)
            : Merit()
            , hpwh(hpwh_in)
            , firstHourRating(firstHourRating_in)
            , standardTestOptions(standardTestOptions_in)

        {
            targetVal = uefMeritInput.targetVal;
            ambientT_C = uefMeritInput.ambientT_C;
        }
        virtual ~UEF_Merit() = default;

        MeritInput::MeritType meritType() override { return MeritInput::MeritType::UEF; }

        void eval() override
        { // get current UEF
            static HPWH::StandardTestSummary standardTestSummary;
            hpwh->customTestOptions.overrideAmbientT = true;
            hpwh->customTestOptions.ambientT_C = ambientT_C;
            hpwh->run24hrTest(*firstHourRating, standardTestSummary, *standardTestOptions);
            currVal = standardTestSummary.UEF;
        }

        void evalDiff(double& diff) override
        { // get difference ratio
            eval();
            diff = (currVal - targetVal) / tolVal;
        }
    };

    std::vector<std::shared_ptr<Fitter::Merit>> pMerits; // could be a vector for add'l FOMs
    std::vector<std::shared_ptr<Fitter::Param>> pParams;
    std::shared_ptr<Courier::Courier> courier = nullptr;

    Fitter(std::vector<std::shared_ptr<Fitter::Merit>> pMerits_in,
           std::vector<std::shared_ptr<Fitter::Param>> pParams_in,
           std::shared_ptr<Courier::Courier> courier_in)
        : pMerits(pMerits_in), pParams(pParams_in), courier(courier_in)
    {
    }

    struct Inverter
    { // invert a 1 x 2 matrix
        static bool getLeftDampedInv(const double nu,
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

            if (abs(det) < thresh)
            {
                return false;
            }

            invMatV.resize(2);
            invMatV[0] = (a * B - b * C) / det;
            invMatV[1] = (-a * C + b * A) / det;
            return true;
        }
    };

    static double targetFunc(void* p0, double& x)
    { // provide single function value
        Fitter* fitter = (Fitter*)p0;
        auto nParams = fitter->pParams.size();
        auto nMerits = fitter->pMerits.size();

        if ((nParams == 1) && (nMerits == 1))
        {
            auto param = fitter->pParams[0];
            auto merit = fitter->pMerits[0];

            *param->val = x;
            merit->eval();
            return merit->currVal;
        }
        return 1.e12;
    }

    //-----------------------------------------------------------------------------
    int secant( // find x given f(x) (secant method)
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

    void leastSquares()
    {
        bool success = false;
        const int maxIters = 20;

        //
        std::vector<double> dParams;
        for (auto& pParam : pParams)
        {
            pParam->assignVal();
            dParams.push_back(pParam->dVal);
        }

        auto pMerit = pMerits[0];

        auto nParams = pParams.size();
        double nu = 0.1;
        for (auto iter = 0; iter < maxIters; ++iter)
        {
            std::string iter_msg = fmt::format("iter {:d}: ", iter);

            pMerit->eval();

            if (pMerit->meritType() == MeritInput::MeritType::UEF)
                iter_msg.append(fmt::format(", UEF: {:g}", pMerit->currVal));

            bool first = true;
            for (std::size_t j = 0; j < nParams; ++j)
            {
                if (!first)
                    iter_msg.append(",");

                iter_msg.append(fmt::format(" param{:d}: {:g}", j, *(pParams[j]->val)));
                first = false;
            }

            double dMerit0 = 0.;
            pMerit->evalDiff(dMerit0);
            double FOM0 = dMerit0 * dMerit0;
            double FOM1 = 0., FOM2 = 0.;
            iter_msg.append(fmt::format(", FOM: {:g}", FOM0));
            courier->send_info(iter_msg);

            std::vector<double> paramV(nParams);
            for (std::size_t i = 0; i < nParams; ++i)
            {
                paramV[i] = *(pParams[i]->val);
            }

            std::vector<double> jacobiV(nParams); // 1 x 2
            for (std::size_t j = 0; j < nParams; ++j)
            {
                *(pParams[j]->val) = paramV[j] + (pParams[j]->dVal);
                double dMerit;
                pMerit->evalDiff(dMerit);
                jacobiV[j] = (dMerit - dMerit0) / (pParams[j]->dVal);
                *(pParams[j]->val) = paramV[j];
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
                    inc1ParamV[j] = -invJacobiV1[j] * dMerit0;
                    paramV1[j] += inc1ParamV[j];
                    *(pParams[j]->val) = paramV1[j];
                }
                double dMerit;
                pMerit->evalDiff(dMerit);
                FOM1 = dMerit * dMerit;

                // restore
                for (std::size_t j = 0; j < nParams; ++j)
                {
                    *(pParams[j]->val) = paramV[j];
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
                    inc2ParamV[j] = -invJacobiV2[j] * dMerit0;
                    paramV2[j] += inc2ParamV[j];
                    *(pParams[j]->val) = paramV2[j];
                }
                double dMerit;
                pMerit->evalDiff(dMerit);
                FOM2 = dMerit * dMerit;

                // restore
                for (std::size_t j = 0; j < nParams; ++j)
                {
                    *(pParams[j]->val) = paramV[j];
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
                            *(pParams[i]->val) = paramV1[i];
                            (pParams[i]->dVal) = inc1ParamV[i] / 1.e3;
                            FOM0 = FOM1;
                        }
                    }
                    else
                    { // pick 2
                        for (std::size_t i = 0; i < nParams; ++i)
                        {
                            *(pParams[i]->val) = paramV2[i];
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
                    courier->send_error("Failure in makeGenericModel using leastSquares");
                }
            }

            if (FOM0 < 1.e-12)
            {
                courier->send_info("Fit converged.");
                success = true;
                break;
            }
        }
        if (!success)
            courier->send_error("Fit did not converge.");
    }

    void fit()
    { // minimize the FOM by varying parameters
        auto nParams = pParams.size();
        auto nMerits = pMerits.size();

        if ((nParams == 1) && (nMerits == 1))
        {
            auto param = pParams[0];
            auto merit = pMerits[0];

            param->assignVal();
            double val0 = *param->val;
            *param->val = val0;
            merit->eval();
            double f0 = merit->currVal;

            double val1 = (1.001) * val0;
            *param->val = val1;
            merit->eval();
            double f1 = merit->currVal;

            int iters =
                secant(targetFunc, this, merit->targetVal, 1.e-12, val0, f0, val1, f1, 1.e-12);
            if (iters < 0)
                courier->send_error("Failure in makeGenericModel using secant");
        }
        else if ((nParams == 2) && (nMerits == 1))
        {
            leastSquares();
        }
        else
        {
            courier->send_error("Cannot perform fit.");
        }
    }
};

#endif
