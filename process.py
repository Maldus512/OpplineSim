import re
import sys
import json
from glob import glob

import numpy as np
import scipy as sp
import scipy.stats

def mean_confidence_interval(data, confidence=0.95):
    a = 1.0*np.array(data)
    n = len(a)
    m, se = np.mean(a), sp.stats.sem(a)
    h = se * sp.stats.t._ppf((1+confidence)/2., n-1)
    return h


def processStats(filename):
    stats = {}
    numHost = 0

    stats['latency'] = []
    stats['acklatency'] = []
    stats['received'] = []
    stats['ackreceived'] = []
    stats['sent'] = []
    stats['acksent'] = []
    stats['acked'] = []
    stats['generated'] = []

    with open(filename, 'r') as f:
        for line in f.readlines():
            res = re.match(r"scalar\s+Field.hosts\[(\d+)\].app\s+#(latency|acklatency|received|ackreceived|sent|acksent|acked|generated)\s+(\d+)", line)
            if res:
                if int(res.group(1)) > numHost:
                    numHost = int(res.group(1))

                stats[res.group(2)].append(int(res.group(3)))

    try:
        prr = (sum(stats['received'])+sum(stats['ackreceived']))/float(sum(stats['sent'])+sum(stats['acksent']))
        prrreq = sum(stats['received'])/float(sum(stats['sent']))
        prrack = sum(stats['ackreceived'])/float(sum(stats['acksent']))
	if not stats['latency']:
		stats['latency'] = 2880*1000
		print("set latency to 'very high' due to absence of received messages")

	if not stats['acklatency']:
		stats['acklatency'] = 2880*1000
		print("set acklatency to 'very high' due to absence of received messages")

	latency = np.mean(stats['latency'])
        acklatency = np.mean(stats['acklatency'])
        dissemination = sum(stats['generated'])/float((sum(stats['sent']) + sum(stats['acksent'])) * numHost)
    except (ZeroDivisionError) as e:
        print("WARNING: some values are not valid")
	print(str(e))
        return {}

    results = {}
    results['prr'] = prr
    results['prrreq'] = prrreq
    results['prrack'] = prrack
    results['latency'] = latency/1000
    results['acklatency'] = acklatency/1000
    results['dissemination'] = dissemination
    return results



if __name__ == '__main__':
    if len(sys.argv) < 2:
        exit(1)

    num = 0
    final = {}
    for filen in glob(sys.argv[1]):
        print("processing {} ...".format(filen))
        res = processStats(filen)

	if not res:
		print("Error in processing {}".format(filen))
		continue

        for key in res:
            if not key in final:
                final[key] = []
                
            final[key].append(res[key])

	num += 1
            

    #print(final["latency"])
    print("Average from {} scalar output files".format(num))
    result = {}
    for key in final:
	result[key+"_var"] = np.var(final[key])
        result[key] = np.mean(final[key])
	result[key+"_conf"] = mean_confidence_interval(final[key])
    
    print(json.dumps(result, indent=4, sort_keys=True))
