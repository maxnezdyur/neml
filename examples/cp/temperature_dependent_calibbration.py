#!/usr/bin/env python3

from Ti_model_make import *
from neml import drivers

import numpy as np
import numpy.linalg as la
import numpy.random as ra
import scipy.interpolate as inter
import scipy.optimize as opt
import matplotlib.pyplot as plt
import pandas as pd

import time
import concurrent.futures
from multiprocessing import Pool
from optimparallel import minimize_parallel

from scipy.optimize import curve_fit


# sets up x_scale for both experiment and simulation
emax = 0.2
Nsample = 200
x_sample = np.linspace(0.0, emax*0.99, Nsample)
#  model grains and threads
erate = 1.0e-2
Ngrains = 200
nthreads = 30
T = 298.0

#================================================#
def make_Ti_model(x_sample, taus_1, taus_2, taus_3,
                  taut_1, taut_2, X_s,
                  k1_1, k1_2, k1_3,
                  k2_1, k2_2, k2_3):
#================================================#

  res = Ti_maker_sim(taus_1, taus_2, taus_3,
            taut_1, taut_2, X_s,
            k1_1, k1_2, k1_3,
            k2_1, k2_2, k2_3,
            T = T, emax = emax, N = Ngrains,
            strain_rate = erate, nthreads = nthreads,
            verbose = True, Taylor = True,
            PTR = True)

  return res
  
#================================================#
def load_file(path):
#================================================#
  fnames = glob.glob(path + "*.csv")
  for f in fnames:
    strain_rate = os.path.basename(f).split('_')[0]
    if strain_rate == "1e-2":
      temp = os.path.basename(f).split('_')[1].split('.')[0]
    if strain_rate == "1e-2" and temp == "298k":
      df = pd.read_csv(f, usecols=[0,1], names=['Nominal_strain', 'True_stress'], header=None)
      return df

"""
#================================================#
def load_file(path, temper):
#================================================#
  for file in glob.glob(path + temper + "k.csv"):
    df = pd.read_csv(file, usecols=[0,1], names=['Nominal_strain', 'True_stress'], header=None)
  return df
"""
"""
#================================================#
def load_file(path, temper):
#================================================#
  for file in glob.glob(path + "CG_Ti.csv"):
    df = pd.read_csv(file, usecols=[0,1], names=['Nominal_strain', 'True_stress'], header=None)
  return df
"""


if __name__ == "__main__":
 
 
  path_1 = "/mnt/c/Users/ladmin/Desktop/argonne/RTRC_data_extract/Huang-2007-MSEA/"
  df = load_file(path_1)

  # room temperature
  params = [190.0, 110.5, 230.0,
        180.0, 250.0, 0.9,
        1.0, 0.25, 5.0,
        220.0, 220.0, 220.0]

  # 423k temperature
  params = [145.0, 70.5, 185.0,
        180.0, 250.0, 0.9,
        1.0, 0.25, 5.0,
        250.0, 250.0, 250.0]
        
  # 523k temperature
  params = [100.0, 50.0, 145.0,
        180.0, 250.0, 0.9,
        1.0, 0.25, 5.0,
        320.0, 320.0, 320.0]

  # 623k temperature
  params = [65.0, 40.0, 115.0,
        180.0, 250.0, 0.9,
        1.0, 0.25, 5.0,
        350.0, 350.0, 350.0]

  res = make_Ti_model(x_sample, *params)
  true_strain = np.log(1+res['strain'])
  true_stress = res['stress'] * (1 + res['strain'])
  plt.plot(true_strain, true_stress, 'g--', label = 'fit')
  plt.plot(df['True_strain'], df['True_stress'], 'b-', label = 'data')
  plt.xlabel('x')
  plt.ylabel('y')
  plt.legend()
  plt.grid(True)
  # plt.savefig("Ti-fitting-simple.png")
  plt.show()
  plt.close()
 
 
  """
  path_1 = "/mnt/c/Users/ladmin/Desktop/argonne/RTRC_data_extract/Nasser-1999-Acta/"
  df = load_file(path_1, str(int(T)))
  
  
  ## Ito-2019-MSEB room temperature
  params = [230.0, 140.5, 260.0,
            240.0, 300.0, 0.9,
            1.5, 0.25, 5.0,
            160.0, 160.0, 160.0]
  
  ## Nasser-1999-Acta
  # room temperature
  params = [100.0, 80.5, 100.0,
        130.0, 200.0, 0.9,
        1.00, 0.15, 4.00,
        16.0, 16.0, 16.0]
  # T = 373k
  params = [95.0, 75.5, 95.0,
        125.0, 195.0, 0.9,
        1.00, 0.15, 4.00,
        25.0, 25.0, 25.0] 
  # T = 473k
  params = [92.0, 72.5, 92.0,
        125.0, 195.0, 0.9,
        1.00, 0.15, 4.00,
        35.0, 35.0, 35.0]
  # T = 573k
  
  res = make_Ti_model(x_sample, *params)
  true_stress = res['stress'] * (1 + res['strain'])
  plt.plot(res['strain'], true_stress, 'g--', label = 'fit')
  plt.plot(df['Nominal_strain'], df['True_stress'], 'b-', label = 'data')
  plt.xlabel('x')
  plt.ylabel('y')
  plt.legend()
  # plt.savefig("Ti-fitting-simple.png")
  plt.show()
  plt.close()
  """

  
