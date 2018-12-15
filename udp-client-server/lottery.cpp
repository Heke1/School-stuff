#include "lottery.h"

lottery::lottery(const unsigned int range_max, const unsigned int  num_lottery, const unsigned int num_extra):
_range_max(range_max),
_num_lottery(num_lottery),
_num_extra (num_extra)

{
    //check that object is constructed to sane state.
    if ( _range_max < 1 or _num_lottery < 1 or _num_extra < 0 )
          throw std::range_error("lottery_cstr(), too small value for parameters");

    if ( (_num_lottery +_num_extra) >=_range_max )
          throw std::range_error("lottery_cstr(), in what lottery is the amount of numbers raffled, larger than range?");

}

std::string  lottery::operator() ()
 {
     // for holding values before they get streamed to return string
     std::vector <int> temp;
     // Lottery number can be raffled only once so we need a mechanism to prevent
     // re-raffle of numbers. Let's define a vector for storing weights.
     std::vector<int> weights;
     //stream for converting numbers to string
     std::ostringstream oss;

     // Exclude zero. It rarely is an option in lotteries.
     // Numbers from 1 to _range max + 1 (we excluded zero) need to have even weights, 1 will do.
     weights.push_back(0);
     for ( int i = 1 ;  i <(_range_max + 1); ++i )
         weights.push_back(1);

     // we need to push generated ints to our temp vector and
     //  change weights of allready raffled numbers to zero.
     for ( int i = 0; i <_num_lottery + _num_extra; ++i ) {
         temp.push_back(get_next_rand(weights));
         weights.at(temp.back()) = 0;
     }

     oss<< "Lottery numbers are: ";
     // copy numbers from vector and separate values with commas.
     std::copy (temp.begin(), temp.end()-(1 + _num_extra), std::ostream_iterator<int> (oss, ","));


     if ( _num_extra > 0 ) {
         // last value of lottery, no comma after it.
         oss << temp.at(temp.size() - (_num_extra +1));

         oss << "\nExtra numbers are: ";
         // Let's do the same deed for extra numbers.
         std::copy (temp.begin() + _num_lottery, temp.end()-1 , std::ostream_iterator<int> (oss, ","));
     }

     // last value, no comma after it.
     oss << temp.back();

     return oss.str();
 }

int lottery::get_next_rand(std::vector<int> &weights)
{
    // dev is for making seeds to latter one. (this is how they do it in cppref demo )
    std::random_device dev;
    // Mersenne Twister for random number generation.
    std::mt19937 gen(dev());

    // The distribution object could not be modified between calls,
    // so we need to create new object for generating random numbers with distribution that changes
    // after every call. So, let's just kill them with '}'.

    std::discrete_distribution <> dist(weights.begin(), weights.end());
    // pass mersenne twister engine as parameter and return the result
    return dist(gen);
}
