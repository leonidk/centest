import os,sys,platform
import json

dataset_dir = '.'
metrics_dir = '.'
algs_dir = '.'
out_dir = '.'

def loadFiles(folder,t):
    res = []
    for f in os.listdir(folder):
        jd = os.path.join(folder,f)
        try:
            with open(jd) as fp:
                d = json.load(fp)
                if d['type'] == t:
                    res.append(d)
        except:
            pass
    return res

datasets = loadFiles(dataset_dir,'dataset')
algorithms = loadFiles(algs_dir,'algorithm')
metrics = loadFiles(metrics_dir,'metric')