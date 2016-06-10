/// Tests for Eigen librairy
///
/// (c) Koheron

#ifndef __EIGEN_TESTS_HPP__
#define __EIGEN_TESTS_HPP__

#include <Eigen/Dense>

class EigenTests
{
  public:
    EigenTests() {}

    void small_vector()
    {
        Eigen::Vector2f v(1, 2);
        
        for(int i=0; i<v.size(); i++)
            printf("%i --> %f", i, v(i));
    }

    void dynamic_vector(unsigned int len)
    {
        Eigen::VectorXf v(len);
        
        for(int i=0; i<v.size(); i++)
            v(i) = i*i;
        
        for(int i=0; i<v.size(); i++)
            printf("%i --> %f", i, v(i));
    }
};

#endif // __TUPLE_TESTS_HPP__
