# RootFinder
A PyQT5 RootFinder with a backend C++ dll

I created this project in a couple days just to experiment with ctypes and play with methods for communication between C++ and python code.

You can turn off logging in the framework.h by setting IS_DEBUG=false if you really wanted to improve speed in the C++ code.

Uses Newton's method to solve for a single root of an equation, given an initial guess, up to a precision given by the user
  For example if you type "x^2-2" it will find the derivative (2*x), and use these two expressions to solve for an approximation of the square root of two iteratively.
  
Made my own expression evaluation tool and derivative solver in C++.
Due to the way that the expression evaluator works with floating point values, the maximum precision is around 1E-5. You could improve this by modifying how floating point values are converted to strings and entered into the equations, or by abandonning the full string-based evaluation approach I used. As this was more of a demo than anything I went with a simple approach here, and I recognize this is one of the flaws of this approach for evaluation. 

