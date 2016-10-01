#!/usr/bin/env python

import os,sys,platform
import json,hashlib
from collections import defaultdict
from util import *
from subprocess import call,check_output
import copy
import numpy as np
def extract_fields(dataset,name,out_dir):
    data = {k: v[name] for k,v in dataset.iteritems() if type(v) == type({})}
    data["left_mono"] = data["data"]["left"]["mono"]
    data["left_rgb"] = data["data"]["left"]["rgb"]
    data["right_mono"] = data["data"]["right"]["mono"]
    data["right_rgb"] = data["data"]["right"]["rgb"]
    data["output_disp"] = os.path.join(out_dir,name + '_disp.pfm')
    data["output_conf"] = os.path.join(out_dir,name + '_conf.pfm')
    data["costs"] = os.path.join(out_dir,name + '_cost.pmm')
    return data

def run_alg(dataset,algorithm,out_dir):
    for name in dataset['names']:
        #print name
        data = extract_fields(dataset,name,out_dir)
        alg_copy = copy.deepcopy(data)
        alg_copy['gt'] = ''
        alg_copy['gt_mask'] = ''
        algorithm.update(alg_copy)
        cfg_path = os.path.join(out_dir,name + '_config.json')
        with open(cfg_path,'w') as fp:
            json.dump(algorithm,fp)

        with cd(algorithm['dir']):
            call([algorithm['command'],cfg_path])

def run_all_algs():
    saved_file = 'finished_results.json'
    metric_results = 'metric_results.json'
    dataset_dir = '.'
    metrics_dir = '.'
    algs_dir = '.'
    out_dir = 'out'

    try:
        with open(saved_file) as fp:
            saved = json.load(fp)
    except:
        saved = defaultdict(str)

    datasets = loadFiles(dataset_dir,'dataset')
    algorithms = loadFiles(algs_dir,'algorithm')
    metrics = loadFiles(metrics_dir,'metric')

    check_and_make_dir(out_dir)

    results = {}
    for algorithm in algorithms:
        results[algorithm['name']] = {}
        for dataset in datasets:
            unique_out = os.path.join(out_dir,algorithm['name'],dataset['name'])
            unique_exists = os.path.exists(unique_out)
            check_and_make_dir(unique_out)

            hashkey = '_'.join([ hashlib.sha1(json.dumps(dataset)).hexdigest(), \
                                hashlib.sha1(json.dumps(algorithm)).hexdigest(), \
                                path_checksum([algorithm['command']] + algorithm['dependent_files']) ])
            if (not unique_exists) or (saved[unique_out] != hashkey):
                run_alg(dataset,algorithm,unique_out)
                saved[unique_out] = hashkey
                print unique_out,hashkey
            result = {}
            for name in dataset['names']:
                data = extract_fields(dataset,name,unique_out)
                result[name] = {}
                for metric in metrics:
                    metric.update(data)
                    metric['output'] = ''

                    #with open(metric_cfg_path,'w') as fp:
                    #    json.dump(metric,fp)
                    #print metric_cfg_path, name, unique_out
                    res = check_output([metric['command'],json.dumps(metric)])
                    resn = json.loads(res)
                    result[name].update(json.loads(res))
                    #print res#json.dumps(metric, sort_keys=True,indent=4, separators=(',', ': '))
                    #with open(metric['output'],'r') as fp:
                    #    res = json.load(fp)
                    #if name in results[unique_name]:
                    #    results[unique_name][name].update(res)
                    #else:
                    #    results[unique_name][name] = res
            unique_name = '_'.join([dataset['name'],algorithm['name']])
            results[algorithm['name']][dataset['name']] = result
    with open(saved_file,'w') as fp:
        json.dump(saved,fp)
    with open(metric_results,'w') as fp:
        json.dump(results,fp)
    return results

if __name__ == '__main__':
    results = run_all_algs()
    short_results = {}
    geomean = lambda n: reduce(lambda x,y: x*y, n) ** (1.0 / len(n))
    for alg,ds in results.iteritems():
        rz = []
        for dsn,de in ds.iteritems():
            for den,res in de.iteritems():
                nd = {k:v for k,v in res.iteritems()}
                rz.append(nd)
        metric_names = rz[0].keys()
        rzz = {m: np.mean([y[m]['result'] for y in rz]) for m in metric_names}
        short_results[alg] = rzz
    print json.dumps(short_results, sort_keys=True,indent=4, separators=(',', ': '))

