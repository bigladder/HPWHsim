#ifndef HPWHTANK_hh
#define HPWHTANK_hh

#include "HPWH.hh"

class HPWH::Tank
{
  public:
    friend class HPWH;

    HPWH* hpwh;

    Tank() {} /**< default constructor, does not create a useful HeatSource */
    Tank(HPWH* hpwh_in) {hpwh = hpwh_in;}

    /**< constructor assigns a pointer to the hpwh that owns this heat source  */
    Tank(const Tank& tank);            /// copy constructor
    Tank& operator=(const Tank& tank); /// assignment operator
    /**< the copy constructor and assignment operator basically just checks if there
        are backup/companion pointers - these can't be copied */

    void init(nlohmann::json j = {}) {std::cout << j;}

}; // end of HPWH::Tank class

#endif
