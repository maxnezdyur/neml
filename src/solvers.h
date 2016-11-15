#ifndef SOLVERS_H
#define SOLVERS_H

#include <cstddef>
#include <memory>

namespace neml {

/// Generic nonlinear solver interface
class Solvable {
 public:
  virtual size_t nparams() const = 0;
  virtual int init_x(double * const x) = 0;
  virtual int RJ(const double * const x, double * const R, double * const J) = 0;
};

// This is entirely for testing
class TestRosenbrock: public Solvable {
 public:
  TestRosenbrock(size_t N);

  virtual size_t nparams() const;
  virtual int init_x(double * const x);
  virtual int RJ(const double * const x, double * const R, double * const J);

 private:
  const size_t N_;
};

/// Call the built-in solver
int solve(std::shared_ptr<Solvable> system, double * x, 
          double tol = 1.0e-8, int miter = 50,
          bool verbose = false);

/// Default solver: plain NR
int newton(std::shared_ptr<Solvable> system, double * x, 
          double tol, int miter, bool verbose);


/// Helper to get numerical jacobian
int diff_jac(std::shared_ptr<Solvable> system, const double * const x,
             double * const nJ, double eps = 1.0e-9);
/// Helper to get checksum
double diff_jac_check(std::shared_ptr<Solvable> system, const double * const x,
                      const double * const J);

} // namespace neml

#endif // SOLVERS_H
