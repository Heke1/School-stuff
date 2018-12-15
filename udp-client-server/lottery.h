#pragma once

#include <vector>
#include <string>
#include <random>
#include <functional>
#include <stdexcept>
#include <algorithm>
#include <sstream>
#include <iterator>
#include <memory>


//implementation of lottery functor.

class lottery {

public:
  lottery(){}
          // biggest number in lottery, how many numbers get raffled, how many extra numbers, 0 if non.
  lottery(const unsigned int range_max, const unsigned int  num_lottery, const unsigned int num_extra);
  std::string  operator () ();


private:

   int get_next_rand(std::vector<int> &weights);

   //in case constructed with default define default-state which is 'lotto' by veikkaus.
   const int _range_max = 39;
   const int _num_lottery = 7;
   const int _num_extra = 2;

};
