#ifndef DAMAGE_H
#define DAMAGE_H

#include "models.h"
#include "elasticity.h"
#include "larsonmiller.h"

#include "windows.h"

#include <memory>

namespace neml {

/// Small strain damage model
class NEML_EXPORT NEMLDamagedModel_sd: public NEMLModel_sd {
 public:
  /// Input is an elastic model, an undamaged base material, and the CTE
  NEMLDamagedModel_sd(ParameterSet & params);

  /// How many history variables?  Equal to base_history + ndamage
  virtual size_t nhist() const;
  /// Initialize base according to the base model and damage according to
  /// init_damage
  virtual void init_hist(double * const hist) const;

  /// The damaged stress update
  virtual void update_sd(
      const double * const e_np1, const double * const e_n,
      double T_np1, double T_n,
      double t_np1, double t_n,
      double * const s_np1, const double * const s_n,
      double * const h_np1, const double * const h_n,
      double * const A_np1,
      double & u_np1, double u_n,
      double & p_np1, double p_n) = 0;

  /// Number of damage variables
  virtual size_t ndamage() const = 0;
  /// Setup the damage variables
  virtual void init_damage(double * const damage) const = 0;

  /// Override the elastic model
  virtual void set_elastic_model(std::shared_ptr<LinearElasticModel> emodel);

 protected:
   std::shared_ptr<NEMLModel_sd> base_;
};

/// Scalar damage trial state
class SDTrialState: public TrialState {
 public:
  virtual ~SDTrialState() {};
  double e_np1[6];
  double e_n[6];
  double T_np1, T_n, t_np1, t_n, u_n, p_n;
  double s_n[6];
  double w_n;
  std::vector<double> h_n;
};

/// Special case where the damage variable is a scalar
class NEML_EXPORT NEMLScalarDamagedModel_sd: public NEMLDamagedModel_sd, public Solvable {
 public:
  /// Parameters are an elastic model, a base model, the CTE, a solver
  /// tolerance, the maximum number of solver iterations, and a verbosity
  /// flag
  NEMLScalarDamagedModel_sd(ParameterSet & params);

  /// Stress update using the scalar damage model
  virtual void update_sd(
      const double * const e_np1, const double * const e_n,
      double T_np1, double T_n,
      double t_np1, double t_n,
      double * const s_np1, const double * const s_n,
      double * const h_np1, const double * const h_n,
      double * const A_np1,
      double & u_np1, double u_n,
      double & p_np1, double p_n);

  /// Equal to 1
  virtual size_t ndamage() const;
  /// Initialize to zero
  virtual void init_damage(double * const damage) const;

  /// Number of parameters for the solver
  virtual size_t nparams() const;
  /// Initialize the solver vector
  virtual void init_x(double * const x, TrialState * ts);
  /// The actual nonlinear residual and Jacobian to solve
  virtual void RJ(const double * const x, TrialState * ts,double * const R,
                 double * const J);
  /// Setup a trial state from known information
  void make_trial_state(const double * const e_np1, const double * const e_n,
                       double T_np1, double T_n, double t_np1, double t_n,
                       const double * const s_n, const double * const h_n,
                       double u_n, double p_n,
                       SDTrialState & tss);

  /// Initial value of damage, overridable for models with singularities
  virtual double d_guess() const {return 0;};

  /// The scalar damage model
  virtual void damage(double d_np1, double d_n,
                     const double * const e_np1, const double * const e_n,
                     const double * const s_np1, const double * const s_n,
                     double T_np1, double T_n,
                     double t_np1, double t_n,
                     double * const dd) const = 0;
  /// Derivative with respect to the damage variable
  virtual void ddamage_dd(double d_np1, double d_n,
                     const double * const e_np1, const double * const e_n,
                     const double * const s_np1, const double * const s_n,
                     double T_np1, double T_n,
                     double t_np1, double t_n,
                     double * const dd) const = 0;
  /// Derivative with respect to the strain
  virtual void ddamage_de(double d_np1, double d_n,
                     const double * const e_np1, const double * const e_n,
                     const double * const s_np1, const double * const s_n,
                     double T_np1, double T_n,
                     double t_np1, double t_n,
                     double * const dd) const = 0;
  /// Derivative with respect to the stress
  virtual void ddamage_ds(double d_np1, double d_n,
                     const double * const e_np1, const double * const e_n,
                     const double * const s_np1, const double * const s_n,
                     double T_np1, double T_n,
                     double t_np1, double t_n,
                     double * const dd) const = 0;

 protected:
  void tangent_(const double * const e_np1, const double * const e_n,
               const double * const s_np1, const double * const s_n,
               double T_np1, double T_n, double t_np1, double t_n,
               double w_np1, double w_n, const double * const A_prime,
               double * const A);
  void ekill_update_(double T_np1, const double * const e_np1, 
                    double * const s_np1, 
                    double * const h_np1, const double * const h_n,
                    double * A_np1, 
                    double & u_np1, double u_n, 
                    double & p_np1, double p_n);

 protected:
  double rtol_;
  double atol_;
  int miter_;
  bool verbose_;
  bool linesearch_;
  bool ekill_;
  double dkill_;
  double sfact_;
};

/// Stack multiple scalar damage models together
class NEML_EXPORT CombinedDamageModel_sd: public NEMLScalarDamagedModel_sd {
 public:
  /// Parameters: elastic model, vector of damage models, the base model
  /// CTE, solver tolerance, solver max iterations, and a verbosity flag
  CombinedDamageModel_sd(ParameterSet & params);

  /// String type for the object system
  static std::string type();
  /// Return the default parameters
  static ParameterSet parameters();
  /// Initialize from a parameter set
  static std::unique_ptr<NEMLObject> initialize(ParameterSet & params);

  /// The combined damage variable
  virtual void damage(double d_np1, double d_n,
                     const double * const e_np1, const double * const e_n,
                     const double * const s_np1, const double * const s_n,
                     double T_np1, double T_n,
                     double t_np1, double t_n,
                     double * const dd) const;
  /// Derivative with respect to damage
  virtual void ddamage_dd(double d_np1, double d_n,
                     const double * const e_np1, const double * const e_n,
                     const double * const s_np1, const double * const s_n,
                     double T_np1, double T_n,
                     double t_np1, double t_n,
                     double * const dd) const;
  /// Derivative with respect to strain
  virtual void ddamage_de(double d_np1, double d_n,
                     const double * const e_np1, const double * const e_n,
                     const double * const s_np1, const double * const s_n,
                     double T_np1, double T_n,
                     double t_np1, double t_n,
                     double * const dd) const;
  /// Derivative with respect to stress
  virtual void ddamage_ds(double d_np1, double d_n,
                     const double * const e_np1, const double * const e_n,
                     const double * const s_np1, const double * const s_n,
                     double T_np1, double T_n,
                     double t_np1, double t_n,
                     double * const dd) const;

  virtual void set_elastic_model(std::shared_ptr<LinearElasticModel> emodel);


 protected:
  const std::vector<std::shared_ptr<NEMLScalarDamagedModel_sd>> models_;
};

static Register<CombinedDamageModel_sd> regCombinedDamageModel_sd;

/// Classical Hayhurst-Leckie-Rabotnov-Kachanov damage
class NEML_EXPORT ClassicalCreepDamageModel_sd: public NEMLScalarDamagedModel_sd {
 public:
  /// Parameters are the elastic model, the parameters A, xi, phi, the
  /// base model, the CTE, the solver tolerance, maximum iterations,
  /// and the verbosity flag.
  ClassicalCreepDamageModel_sd(ParameterSet & params);

  /// String type for the object system
  static std::string type();
  /// Return the default parameters
  static ParameterSet parameters();
  /// Initialize from a parameter set
  static std::unique_ptr<NEMLObject> initialize(ParameterSet & params);

  /// The damage function d_np1 = d_n + (se / A)**xi (1 - d_np1)**(-phi) * dt
  virtual void damage(double d_np1, double d_n,
                     const double * const e_np1, const double * const e_n,
                     const double * const s_np1, const double * const s_n,
                     double T_np1, double T_n,
                     double t_np1, double t_n,
                     double * const dd) const;
  /// Derivative of damage wrt damage
  virtual void ddamage_dd(double d_np1, double d_n,
                     const double * const e_np1, const double * const e_n,
                     const double * const s_np1, const double * const s_n,
                     double T_np1, double T_n,
                     double t_np1, double t_n,
                     double * const dd) const;
  /// Derivative of damage wrt strain
  virtual void ddamage_de(double d_np1, double d_n,
                     const double * const e_np1, const double * const e_n,
                     const double * const s_np1, const double * const s_n,
                     double T_np1, double T_n,
                     double t_np1, double t_n,
                     double * const dd) const;
  /// Derivative of damage wrt stress
  virtual void ddamage_ds(double d_np1, double d_n,
                     const double * const e_np1, const double * const e_n,
                     const double * const s_np1, const double * const s_n,
                     double T_np1, double T_n,
                     double t_np1, double t_n,
                     double * const dd) const;

 protected:
  double se(const double * const s) const;

 protected:
  std::shared_ptr<Interpolate> A_;
  std::shared_ptr<Interpolate> xi_;
  std::shared_ptr<Interpolate> phi_;
};

static Register<ClassicalCreepDamageModel_sd> regClassicalCreepDamageModel_sd;

/// Base class of modular effective stresses used by ModularCreepDamageModel_sd
class NEML_EXPORT EffectiveStress: public NEMLObject {
 public:
  EffectiveStress(ParameterSet & params);
  virtual void effective(const double * const s, double & eff) const = 0;
  virtual void deffective(const double * const s, double * const deff) const = 0;
};

/// von Mises stress
class NEML_EXPORT VonMisesEffectiveStress: public EffectiveStress
{
 public:
  VonMisesEffectiveStress(ParameterSet & params);

  /// String type for the object system
  static std::string type();
  /// Return the default parameters
  static ParameterSet parameters();
  /// Initialize from a parameter set
  static std::unique_ptr<NEMLObject> initialize(ParameterSet & params);

  virtual void effective(const double * const s, double & eff) const;
  virtual void deffective(const double * const s, double * const deff) const;
};

static Register<VonMisesEffectiveStress> regVonMisesEffectiveStress;

/// Mean stress
class NEML_EXPORT MeanEffectiveStress: public EffectiveStress
{
 public:
  MeanEffectiveStress(ParameterSet & params);

  /// String type for the object system
  static std::string type();
  /// Return the default parameters
  static ParameterSet parameters();
  /// Initialize from a parameter set
  static std::unique_ptr<NEMLObject> initialize(ParameterSet & params);

  virtual void effective(const double * const s, double & eff) const;
  virtual void deffective(const double * const s, double * const deff) const;
};

static Register<MeanEffectiveStress> regMeanEffectiveStress;

/// Huddleston stress
class NEML_EXPORT HuddlestonEffectiveStress: public EffectiveStress
{
 public:
  HuddlestonEffectiveStress(ParameterSet & params);

  /// String type for the object system
  static std::string type();
  /// Return the default parameters
  static ParameterSet parameters();
  /// Initialize from a parameter set
  static std::unique_ptr<NEMLObject> initialize(ParameterSet & params);

  virtual void effective(const double * const s, double & eff) const;
  virtual void deffective(const double * const s, double * const deff) const;

 private:
  double b_;
};

static Register<HuddlestonEffectiveStress> regHuddlestonEffectiveStress;

/// Maximum principal stress
class NEML_EXPORT MaxPrincipalEffectiveStress: public EffectiveStress
{
 public:
  MaxPrincipalEffectiveStress(ParameterSet & params);

  /// String type for the object system
  static std::string type();
  /// Return the default parameters
  static ParameterSet parameters();
  /// Initialize from a parameter set
  static std::unique_ptr<NEMLObject> initialize(ParameterSet & params);

  virtual void effective(const double * const s, double & eff) const;
  virtual void deffective(const double * const s, double * const deff) const;
};

static Register<MaxPrincipalEffectiveStress> regMaxPrincipalEffectiveStress;

/// Maximum of several effective stress measures
class NEML_EXPORT MaxSeveralEffectiveStress: public EffectiveStress
{
 public:
  MaxSeveralEffectiveStress(ParameterSet & params);

  /// String type for the object system
  static std::string type();
  /// Return the default parameters
  static ParameterSet parameters();
  /// Initialize from a parameter set
  static std::unique_ptr<NEMLObject> initialize(ParameterSet & params);

  virtual void effective(const double * const s, double & eff) const;
  virtual void deffective(const double * const s, double * const deff) const;

 private:
  void select_(const double * const s, size_t & ind, double & value) const;

 private:
  std::vector<std::shared_ptr<EffectiveStress>> measures_;
};

static Register<MaxSeveralEffectiveStress> regMaxSeveralEffectiveStress;

/// Weighted some of several effective stresses
class NEML_EXPORT SumSeveralEffectiveStress: public EffectiveStress
{
 public:
  SumSeveralEffectiveStress(ParameterSet & params);

  /// String type for the object system
  static std::string type();
  /// Return the default parameters
  static ParameterSet parameters();
  /// Initialize from a parameter set
  static std::unique_ptr<NEMLObject> initialize(ParameterSet & params);

  virtual void effective(const double * const s, double & eff) const;
  virtual void deffective(const double * const s, double * const deff) const;

 private:
  std::vector<std::shared_ptr<EffectiveStress>> measures_;
  std::vector<double> weights_;
};

static Register<SumSeveralEffectiveStress> regSumSeveralEffectiveStress;

/// Modular version of Hayhurst-Leckie-Rabotnov-Kachanov damage
//    This model differs from the above in two ways
//      1) You can change the effective stress measure
//      2) There is an extra (1-w)^xi term in the formulation to make the
//         results match the old analytic solutions
class NEML_EXPORT ModularCreepDamageModel_sd: public NEMLScalarDamagedModel_sd {
 public:
  ModularCreepDamageModel_sd(ParameterSet & params);

  /// String type for the object system
  static std::string type();
  /// Return the default parameters
  static ParameterSet parameters();
  /// Initialize from a parameter set
  static std::unique_ptr<NEMLObject> initialize(ParameterSet & params);

  /// The damage function d_np1 = d_n + (se / A)**xi * (1-d_np1)**xi * (1 - d_np1)**(-phi) * dt
  virtual void damage(double d_np1, double d_n,
                     const double * const e_np1, const double * const e_n,
                     const double * const s_np1, const double * const s_n,
                     double T_np1, double T_n,
                     double t_np1, double t_n,
                     double * const dd) const;
  /// Derivative of damage wrt damage
  virtual void ddamage_dd(double d_np1, double d_n,
                     const double * const e_np1, const double * const e_n,
                     const double * const s_np1, const double * const s_n,
                     double T_np1, double T_n,
                     double t_np1, double t_n,
                     double * const dd) const;
  /// Derivative of damage wrt strain
  virtual void ddamage_de(double d_np1, double d_n,
                     const double * const e_np1, const double * const e_n,
                     const double * const s_np1, const double * const s_n,
                     double T_np1, double T_n,
                     double t_np1, double t_n,
                     double * const dd) const;
  /// Derivative of damage wrt stress
  virtual void ddamage_ds(double d_np1, double d_n,
                     const double * const e_np1, const double * const e_n,
                     const double * const s_np1, const double * const s_n,
                     double T_np1, double T_n,
                     double t_np1, double t_n,
                     double * const dd) const;

 protected:
  std::shared_ptr<Interpolate> A_;
  std::shared_ptr<Interpolate> xi_;
  std::shared_ptr<Interpolate> phi_;
  std::shared_ptr<EffectiveStress> estress_;
};

static Register<ModularCreepDamageModel_sd> regModularCreepDamageModel_sd;

/// Time-fraction ASME damage using a generic Larson-Miller relation and effective stress
class NEML_EXPORT LarsonMillerCreepDamageModel_sd: public NEMLScalarDamagedModel_sd {
 public:
  LarsonMillerCreepDamageModel_sd(ParameterSet & params);
  
  /// String type for the object system
  static std::string type();
  /// Return the default parameters
  static ParameterSet parameters();
  /// Initialize from a parameter set
  static std::unique_ptr<NEMLObject> initialize(ParameterSet & params);
  
  /// The damage function d_np1 = d_n + 1/tr(s*(1-w), T) * dt
  virtual void damage(double d_np1, double d_n, 
                     const double * const e_np1, const double * const e_n,
                     const double * const s_np1, const double * const s_n,
                     double T_np1, double T_n,
                     double t_np1, double t_n,
                     double * const dd) const;
  /// Derivative of damage wrt damage
  virtual void ddamage_dd(double d_np1, double d_n, 
                     const double * const e_np1, const double * const e_n,
                     const double * const s_np1, const double * const s_n,
                     double T_np1, double T_n,
                     double t_np1, double t_n,
                     double * const dd) const;
  /// Derivative of damage wrt strain
  virtual void ddamage_de(double d_np1, double d_n, 
                     const double * const e_np1, const double * const e_n,
                     const double * const s_np1, const double * const s_n,
                     double T_np1, double T_n,
                     double t_np1, double t_n,
                     double * const dd) const;
  /// Derivative of damage wrt stress
  virtual void ddamage_ds(double d_np1, double d_n, 
                     const double * const e_np1, const double * const e_n,
                     const double * const s_np1, const double * const s_n,
                     double T_np1, double T_n,
                     double t_np1, double t_n,
                     double * const dd) const;

 protected:
  std::shared_ptr<LarsonMillerRelation> lmr_;
  std::shared_ptr<EffectiveStress> estress_;
};

static Register<LarsonMillerCreepDamageModel_sd> regLarsonMillerCreepDamageModel_sd;

/// A standard damage model where the damage rate goes as the plastic strain
class NEML_EXPORT NEMLStandardScalarDamagedModel_sd: public NEMLScalarDamagedModel_sd {
 public:
  /// Parameters: elastic model, base model, CTE, solver tolerance,
  /// solver maximum number of iterations, verbosity flag
  NEMLStandardScalarDamagedModel_sd(ParameterSet & params);

  /// Damage, now only proportional to the inelastic effective strain
  virtual void damage(double d_np1, double d_n,
                     const double * const e_np1, const double * const e_n,
                     const double * const s_np1, const double * const s_n,
                     double T_np1, double T_n,
                     double t_np1, double t_n,
                     double * const dd) const;
  /// Derivative of damage wrt damage
  virtual void ddamage_dd(double d_np1, double d_n,
                     const double * const e_np1, const double * const e_n,
                     const double * const s_np1, const double * const s_n,
                     double T_np1, double T_n,
                     double t_np1, double t_n,
                     double * const dd) const;
  /// Derivative of damage wrt strain
  virtual void ddamage_de(double d_np1, double d_n,
                     const double * const e_np1, const double * const e_n,
                     const double * const s_np1, const double * const s_n,
                     double T_np1, double T_n,
                     double t_np1, double t_n,
                     double * const dd) const;
  /// Derivative of damage wrt stress
  virtual void ddamage_ds(double d_np1, double d_n,
                     const double * const e_np1, const double * const e_n,
                     const double * const s_np1, const double * const s_n,
                     double T_np1, double T_n,
                     double t_np1, double t_n,
                     double * const dd) const;

  /// The part of the damage rate proportional to the inelastic strain rate
  virtual void f(const double * const s_np1, double d_np1,
                double T_np1, double & f) const = 0;
  /// Derivative with respect to stress
  virtual void df_ds(const double * const s_np1, double d_np1,
                   double T_np1, double * const df) const = 0;
  /// Derivative with respect to damage
  virtual void df_dd(const double * const s_np1, double d_np1,
                   double T_np1, double & df) const = 0;

 protected:
  double dep(const double * const s_np1, const double * const s_n,
             const double * const e_np1, const double * const e_n,
             double T_np1) const;

};

/// The isothermal form of my pet work-based damage model
class NEML_EXPORT NEMLWorkDamagedModel_sd: public NEMLScalarDamagedModel_sd {
 public:
  /// Parameters: elastic model, base model, CTE, solver tolerance,
  /// solver maximum number of iterations, verbosity flag
  NEMLWorkDamagedModel_sd(ParameterSet & params);

  /// String type for the object system
  static std::string type();
  /// Return the default parameters
  static ParameterSet parameters();
  /// Initialize from a parameter set
  static std::unique_ptr<NEMLObject> initialize(ParameterSet & params);

/// Initial value of damage, overridable for models with singularities
  virtual double d_guess() const {return eps_;};

  /// damage rate = n * d**((n-1)/n) * W_dot / W_crit
  virtual void damage(double d_np1, double d_n,
                     const double * const e_np1, const double * const e_n,
                     const double * const s_np1, const double * const s_n,
                     double T_np1, double T_n,
                     double t_np1, double t_n,
                     double * const dd) const;
  /// Derivative of damage wrt damage
  virtual void ddamage_dd(double d_np1, double d_n,
                     const double * const e_np1, const double * const e_n,
                     const double * const s_np1, const double * const s_n,
                     double T_np1, double T_n,
                     double t_np1, double t_n,
                     double * const dd) const;
  /// Derivative of damage wrt strain
  virtual void ddamage_de(double d_np1, double d_n,
                     const double * const e_np1, const double * const e_n,
                     const double * const s_np1, const double * const s_n,
                     double T_np1, double T_n,
                     double t_np1, double t_n,
                     double * const dd) const;
  /// Derivative of damage wrt stress
  virtual void ddamage_ds(double d_np1, double d_n,
                     const double * const e_np1, const double * const e_n,
                     const double * const s_np1, const double * const s_n,
                     double T_np1, double T_n,
                     double t_np1, double t_n,
                     double * const dd) const;

 protected:
  double workrate(
      const double * const strain_np1, const double * const strain_n,
      const double * const stress_np1, const double * const stress_n,
      double T_np1, double T_n, double t_np1, double t_n,
      double d_np1, double d_n) const;
                  
 protected:
  std::shared_ptr<Interpolate> Wcrit_;
  double n_;
  double eps_;
};

static Register<NEMLWorkDamagedModel_sd> regNEMLWorkDamagedModel_sd;

/// Simple power law damage
class NEML_EXPORT NEMLPowerLawDamagedModel_sd: public NEMLStandardScalarDamagedModel_sd {
 public:
  /// Parameters are an elastic model, the constants A and a, the base
  /// material model, the CTE, a solver tolerance, solver maximum number
  /// of iterations, and a verbosity flag
  NEMLPowerLawDamagedModel_sd(ParameterSet & params);

  /// String type for the object system
  static std::string type();
  /// Return the default parameters
  static ParameterSet parameters();
  /// Initialize from a parameter set
  static std::unique_ptr<NEMLObject> initialize(ParameterSet & params);

  /// Damage = A * s_eq**a (times the inelastic strain rate)
  virtual void f(const double * const s_np1, double d_np1,
                double T_np1, double & f) const;
  /// Derivative of f wrt stress
  virtual void df_ds(const double * const s_np1, double d_np1, double T_np1,
                double * const df) const;
  /// Derivative of f wrt damage
  virtual void df_dd(const double * const s_np1, double d_np1, double T_np1,
                double & df) const;

 protected:
  double se(const double * const s) const;

 protected:
  std::shared_ptr<Interpolate> A_;
  std::shared_ptr<Interpolate> a_;
};

static Register<NEMLPowerLawDamagedModel_sd> regNEMLPowerLawDamagedModel_sd;

/// Simple exponential damage model
class NEML_EXPORT NEMLExponentialWorkDamagedModel_sd: public NEMLStandardScalarDamagedModel_sd {
 public:
  /// Parameters are the elastic model, parameters W0, k0, and af, the
  /// base material model, the CTE, a solver tolerance, maximum number
  /// of iterations, and a verbosity flag.
  NEMLExponentialWorkDamagedModel_sd(ParameterSet & params);

  /// String type for the object system
  static std::string type();
  /// Return the default parameters
  static ParameterSet parameters();
  /// Initialize from a parameter set
  static std::unique_ptr<NEMLObject> initialize(ParameterSet & params);

  /// damage rate is (d + k0)**af / W0 * s_eq
  virtual void f(const double * const s_np1, double d_np1,
                double T_np1, double & f) const;
  /// Derivative of damage wrt stress
  virtual void df_ds(const double * const s_np1, double d_np1, double T_np1,
                double * const df) const;
  /// Derivative of damage wrt damage
  virtual void df_dd(const double * const s_np1, double d_np1, double T_np1,
                double & df) const;

 protected:
  double se(const double * const s) const;

 protected:
  std::shared_ptr<Interpolate> W0_;
  std::shared_ptr<Interpolate> k0_;
  std::shared_ptr<Interpolate> af_;
};

static Register<NEMLExponentialWorkDamagedModel_sd> regNEMLExponentialWorkDamagedModel_sd;

} //namespace neml

#endif // DAMAGE_H
