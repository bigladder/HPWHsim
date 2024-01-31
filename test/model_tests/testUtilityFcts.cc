/*This is not a substitute for a proper HPWH Test Tool, it is merely a short program
 * to aid in the testing of the new HPWH.cc as it is being written.
 *
 * -NDK
 *
 * Bring on the HPWH Test Tool!!! -MJL
 *
 *
 *
 */
#include "HPWH.hh"
#include <iostream>
#include <string>

using std::cout;
using std::string;

#define ASSERTTRUE(input, ...)                                                                     \
    if (!(input))                                                                                  \
    {                                                                                              \
        cout << "Assertation failed at " << __FILE__ << ", line: " << __LINE__ << ".\n";           \
        exit(1);                                                                                   \
    }
#define ASSERTFALSE(input, ...)                                                                    \
    if ((input))                                                                                   \
    {                                                                                              \
        cout << "Assertation failed at " << __FILE__ << ", line: " << __LINE__ << ".\n";           \
        exit(1);                                                                                   \
    }

// Compare doubles
bool cmpd(double A, double B, double epsilon = 0.0001) { return (fabs(A - B) < epsilon); }
// Relative Compare doubles
bool relcmpd(double A, double B, double epsilon = 0.00001)
{
    return fabs(A - B) < (epsilon * (fabs(A) < fabs(B) ? fabs(B) : fabs(A)));
}
