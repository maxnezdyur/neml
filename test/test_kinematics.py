#!/usr/bin/env python3

from neml import history, interpolate, elasticity
from neml.math import tensors, rotations
from neml.cp import crystallography, slipharden, sliprules, inelasticity, kinematics

import common
from common import differentiate
from nicediff import *

import unittest
import numpy as np
import numpy.linalg as la

class CommonKinematics(object):
  def test_d_stress_rate_d_stress(self):
    nd = diff_symmetric_symmetric(lambda s: self.model.stress_rate(s, 
      self.d, self.w, self.Q, self.H, self.L, self.T), self.S)

    d = self.model.d_stress_rate_d_stress(self.S, self.d, self.w, self.Q, 
        self.H, self.L, self.T)

    self.assertEqual(d, nd)

  def test_d_stress_rate_d_d(self):
    def dfn(d):
      self.model.decouple(self.S, d, self.w, self.Q, self.H, self.L, self.T)
      return self.model.stress_rate(self.S, d, self.w, self.Q, self.H, self.L, self.T)

    nd = diff_symmetric_symmetric(dfn, self.d)
    d = self.model.d_stress_rate_d_d(self.S, self.d, self.w, self.Q, self.H, self.L,
        self.T)

    self.assertEqual(nd, d)

  def test_d_stress_rate_d_w(self):
    def dfn(w):
      self.model.decouple(self.S, self.d, w, self.Q, self.H, self.L, self.T)
      return self.model.stress_rate(self.S, self.d, w, self.Q, self.H, self.L, self.T)

    nd = diff_symmetric_skew(dfn, self.w)
    d = self.model.d_stress_rate_d_w(self.S, self.d, self.w, self.Q, self.H,
        self.L, self.T)
   
    Cfull = self.emodel.C_tensor(self.T, self.Q)
    Sfull = self.emodel.S_tensor(self.T, self.Q)

    e = Sfull * self.S

    def wee(w):
      os = self.model.spin(self.S, self.d, tensors.Skew(common.uskew(w)), 
          self.Q, self.H, self.L, self.T) + self.imodel.w_p(
              self.S, self.Q, self.H, self.L, self.T)

      net = tensors.Symmetric(e*os - os*e)

      return -(Cfull * net).data 

    print(differentiate(wee, self.w.data))
    print(tensors.SpecialSymSymSym(Cfull, e))
    
    self.assertEqual(nd, d)

  def test_d_stress_rate_d_history(self):
    nd = diff_symmetric_history(lambda h: self.model.stress_rate(self.S, self.d,
      self.w, self.Q, h, self.L, self.T), self.H)
    d = self.model.d_stress_rate_d_history(self.S, self.d, self.w, self.Q, self.H,
        self.L, self.T)

    self.assertTrue(np.allclose(nd, np.array(d).reshape(nd.shape)))

  def test_d_history_rate_d_stress(self):
    nd = diff_history_symmetric(lambda s: self.model.history_rate(s, self.d,
      self.w, self.Q, self.H, self.L, self.T), self.S)
    d = np.array(self.model.d_history_rate_d_stress(self.S, self.d, self.w,
        self.Q, self.H, self.L, self.T))

    self.assertTrue(np.allclose(nd, d.reshape(nd.shape)))

  def test_d_history_rate_d_d(self):
    nd = diff_history_symmetric(lambda d: self.model.history_rate(self.S, d,
      self.w, self.Q, self.H, self.L, self.T), self.d)
    d = np.array(self.model.d_history_rate_d_d(self.S, self.d, self.w, self.Q,
        self.H, self.L, self.T))

    self.assertTrue(np.allclose(nd, d.reshape(nd.shape)))

  def test_d_history_rate_d_w(self):
    nd = diff_history_skew(lambda w: self.model.history_rate(self.S, self.d,
      w, self.Q, self.H, self.L, self.T), self.w)
    d = np.array(self.model.d_history_rate_d_w(self.S, self.d, self.w, self.Q,
      self.H, self.L, self.T))

    self.assertTrue(np.allclose(nd, d.reshape(nd.shape)))

  def test_d_history_rate_d_history(self):
    nd = diff_history_history(lambda h: self.model.history_rate(self.S, self.d,
      self.w, self.Q, h, self.L, self.T), self.H)
    d = np.array(self.model.d_history_rate_d_history(self.S, self.d, self.w,
      self.Q, self.H, self.L, self.T))

    self.assertTrue(np.allclose(nd, d.reshape(nd.shape)))

class TestStandardKinematics(unittest.TestCase, CommonKinematics):
  def setUp(self):
    self.strength = 35.0
    self.H = history.History()
    self.H.add_scalar("strength")
    self.H.set_scalar("strength", self.strength)

    self.tau0 = 10.0
    self.tau_sat = 50.0
    self.b = 2.5

    self.strengthmodel = slipharden.VoceSlipHardening(self.tau_sat, self.b, self.tau0)
    
    self.g0 = 1.0
    self.n = 3.0
    self.slipmodel = sliprules.PowerLawSlipRule(self.strengthmodel, self.g0, self.n)

    self.imodel = inelasticity.AsaroInelasticity(self.slipmodel)

    self.L = crystallography.CubicLattice(1.0)
    self.L.add_slip_system([1,1,0],[1,1,1])
    
    self.Q = rotations.Orientation(35.0,17.0,14.0, angle_type = "degrees")
    self.S = tensors.Symmetric(np.array([
      [100.0,-25.0,10.0],
      [-25.0,-17.0,15.0],
      [10.0,  15.0,35.0]]))

    self.T = 300.0

    self.mu = 29000.0
    self.E = 120000.0
    self.nu = 0.3

    self.emodel = elasticity.CubicLinearElasticModel(self.E, 
        self.nu, self.mu, "moduli")

    self.dn = np.array([[4.1,2.8,-1.2],[3.1,7.1,0.2],[4,2,3]])
    self.dn = 0.5*(self.dn + self.dn.T)
    self.d = tensors.Symmetric(self.dn)

    self.wn = np.array([[-9.36416517,  2.95527444,  8.70983194],
           [-1.54693052,  8.7905658 , -5.10895168],
           [-8.52740468, -0.7741642 ,  2.89544992]])
    self.wn = 0.5 * (self.wn - self.wn.T)
    self.w = tensors.Skew(self.wn)

    self.model = kinematics.StandardKinematicModel(self.emodel, self.imodel)
    
    self.fspin = self.model.spin(self.S, self.d, self.w, self.Q, self.H,
        self.L, self.T)

    self.model.decouple(self.S, self.d, self.w, self.Q, self.H, self.L, self.T)

  def test_setup_history(self):
    H1 = history.History()
    self.model.populate_history(H1)
    self.model.init_history(H1)

    H2 = history.History()
    self.imodel.populate_history(H2)
    self.imodel.init_history(H2)

    self.assertTrue(np.allclose(np.array(H1), np.array(H2)))

  def test_stress_rate(self):
    Cfull = ms2ts(self.emodel.C_tensor(self.T, self.Q).data.reshape((6,6)))
    Sfull = ms2ts(self.emodel.S_tensor(self.T, self.Q).data.reshape((6,6)))
    d = usym(self.d.data)
    w = uskew(self.w.data)
    Ofull = uskew(self.fspin.data)
    
    dp = usym(self.imodel.d_p(self.S, self.Q, self.H, self.L, self.T).data)
    wp = uskew(self.imodel.w_p(self.S, self.Q, self.H, self.L, self.T).data)

    O = wp + Ofull

    stress = usym(self.S.data)
    
    e = np.einsum('ijkl,kl', Sfull, stress)

    sdot1 = tensors.Symmetric(
        np.einsum('ijkl,kl', Cfull, d - dp - np.dot(e, O) + np.dot(O, e)))
    
    sdot2 = self.model.stress_rate(self.S, self.d, self.w, self.Q, self.H,
        self.L, self.T)

    self.assertEqual(sdot1, sdot2)

  def test_hist_rate(self):
    H1 = self.model.history_rate(self.S, self.d, self.w,
        self.Q, self.H, self.L, self.T)
    H2 = self.imodel.history_rate(self.S, self.Q, self.H, self.L, self.T)

    self.assertTrue(np.allclose(np.array(H1), np.array(H2)))

  def test_spin_rate(self):
    Cfull = ms2ts(self.emodel.C_tensor(self.T, self.Q).data.reshape((6,6)))
    Sfull = ms2ts(self.emodel.S_tensor(self.T, self.Q).data.reshape((6,6)))
    d = usym(self.d.data)
    w = uskew(self.w.data)
    Ofull = uskew(self.fspin.data)
    
    dp = usym(self.imodel.d_p(self.S, self.Q, self.H, self.L, self.T).data)
    wp = uskew(self.imodel.w_p(self.S, self.Q, self.H, self.L, self.T).data)

    O = wp + Ofull

    stress = usym(self.S.data)
    
    e = np.einsum('ijkl,kl', Sfull, stress)

    spin1 = tensors.Skew(
        w - wp - np.dot(e, dp) + np.dot(dp, e))
    spin2 = self.model.spin(self.S, self.d, self.w, self.Q, self.H, 
        self.L, self.T)

    self.assertTrue(spin1, spin2)
