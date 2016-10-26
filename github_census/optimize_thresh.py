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
from opentuner import LogFloatParameter
from opentuner import MeasurementInterface
from opentuner import Result
import run_all
import json
import numpy as np
class DTFlagsTuner(MeasurementInterface):

  def manipulator(self):
    manipulator = ConfigurationManipulator()
    manipulator.add_parameter(IntegerParameter("left_right_int", 0,3))
    manipulator.add_parameter(FloatParameter("left_right_sub", 0.1,1.0))
    manipulator.add_parameter(IntegerParameter("neighbor", 0,100))
    manipulator.add_parameter(IntegerParameter("second_peak", 0,100))
    manipulator.add_parameter(IntegerParameter("texture_diff", 0,40))
    manipulator.add_parameter(IntegerParameter("texture_count", 0,20))
    manipulator.add_parameter(IntegerParameter("score_min", 0,200))
    manipulator.add_parameter(IntegerParameter("score_max", 100,1024))
    manipulator.add_parameter(IntegerParameter("median_plus", 1,15))
    manipulator.add_parameter(IntegerParameter("median_minus", 1,15))
    manipulator.add_parameter(IntegerParameter("median_thresh", 0,250))
    return manipulator

  def run(self, desired_result, input, limit):
    cfg = desired_result.configuration.data

    with open('github_census.json') as fp:
        alg_cfg = json.load(fp)
    alg_cfg['config'].update(cfg)
    with open('github_census.json','w') as fp:
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
    return Result(time=(1.0-short_results['github_census']['f_1'])*100.0)

  def save_final_config(self, configuration):
    print "Optimal block size written to mmm_final_config.json:", configuration.data
    self.manipulator().save_to_file(configuration.data,
                                    'mmm_final_config.json')

if __name__ == '__main__':
  argparser = opentuner.default_argparser()
  DTFlagsTuner.main(argparser.parse_args())
