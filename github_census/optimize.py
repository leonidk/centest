#!/usr/bin/env python
#
# Optimize blocksize of apps/mmm_block.cpp
#
# This is an extremely simplified version meant only for tutorials
#

import opentuner
from opentuner import ConfigurationManipulator
from opentuner import IntegerParameter
from opentuner import FloatParameter
from opentuner import MeasurementInterface
from opentuner import Result
import run_all
import json
import numpy as np
class DTFlagsTuner(MeasurementInterface):

  def manipulator(self):
    manipulator = ConfigurationManipulator()
    manipulator.add_parameter(
      FloatParameter('dt_space', 1, 50))
    manipulator.add_parameter(
      FloatParameter('dt_range', 1, 200))
    return manipulator

  def run(self, desired_result, input, limit):
    cfg = desired_result.configuration.data

    with open('github_dt.json') as fp:
        alg_cfg = json.load(fp)
    alg_cfg['config'].update(cfg)
    with open('opt.json','w') as fp:
        alg_cfg = json.dump(alg_cfg,fp)
    results = run_all.run_all_algs()
    short_results = {}
    for alg,ds in results.iteritems():
        rz = []
        for dsn,de in ds.iteritems():
            for den,res in de.iteritems():
                nd = {k:v for k,v in res.iteritems()}
                rz.append(nd)
        metric_names = rz[0].keys()
        rzz = {m: np.mean([y[m]['result'] for y in rz]) for m in metric_names}
        short_results[alg] = rzz
    return Result(time=short_results['opt']['err_3']*100.0)

  def save_final_config(self, configuration):
    print "Optimal block size written to mmm_final_config.json:", configuration.data
    self.manipulator().save_to_file(configuration.data,
                                    'mmm_final_config.json')

if __name__ == '__main__':
  argparser = opentuner.default_argparser()
  DTFlagsTuner.main(argparser.parse_args())
