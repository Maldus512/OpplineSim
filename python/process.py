import re
import sys
import json
from glob import glob
from os import listdir, remove
from os.path import isfile, join, basename, normpath

import numpy as np
import scipy as sp
import scipy.stats

SIZES = [223]#, 335]
DEVICES = [10]#, 20]
TMI = [60, 540, 900]

def getTestNames(name=""):
    res = []
    for s in SIZES:
    	for d in DEVICES:
            for t in TMI:
                 res.append("{}{}mx{}m_{}_{}".format(name,s,s,d,t))
    return res


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
        
        if prr == 0 or prrreq == 0 or prrack == 0 or prr > 1 or prrreq > 1 or prrack > 1:
            print("Ratios should not be zero (the protocol isn't that bad) or over 1; file: {}".format(filename))
            print(prr, prrreq, prrack)
            print(stats['received'])

        if not stats['latency']:
            stats['latency'] = [2880*1000]
            print("set latency to 'very high' due to absence of received messages")

        if not stats['acklatency']:
            stats['acklatency'] = [2880*1000]
            print("set acklatency to 'very high' due to absence of received messages")

        latency = stats['latency']
        acklatency = stats['acklatency']
        dissemination = sum(stats['generated'])/float((sum(stats['sent']) + sum(stats['acksent'])) * numHost)
    except (ZeroDivisionError) as e:
        print("WARNING: some values are not valid")
	print(str(e))
        return {}

    results = {}
    results['prr'] = prr
    results['prrreq'] = prrreq
    results['prrack'] = prrack
    results['latency'] = [x/1000 for x in latency]
    results['acklatency'] = [x/1000 for x in acklatency]
    results['dissemination'] = dissemination
    resume = {'sent':sum(stats['sent']), 'acksent':sum(stats['acksent']), 'received':sum(stats['received']), 'ackreceived':sum(stats['ackreceived']) }
    return results, resume



if __name__ == '__main__':
    if len(sys.argv) < 2:
        exit(1)
    
    if len(sys.argv) > 2:
        name = sys.argv[2]
        test_folders = getTestNames(name)
    else:
        name = ""
        test_folders = getTestNames()

    for f in glob("[0-9]*x[0-9]*-[0-9]*.json"):
        remove(f)

    for f in glob("[0-9]*x[0-9]*-[0-9]*-sum.json"):
        remove(f)

    for test in test_folders:
        num = 0
        final = {}
        resume_final = {}
        folder = join(sys.argv[1], test)
        print("processing folder {}".format(folder))
        onlyfiles = [f for f in listdir(folder) if isfile(join(folder,f)) and f.endswith(".sca")]
        for f in onlyfiles:
            res, resume = processStats(join(folder,f))
            if not res:
                print("Error in processing {}".format(f))
                continue

            for key in res:
                if not key in final:
                    final[key] = []
                    
                if key == 'latency' or key == 'acklatency':
                    final[key].extend(res[key])
                else:
                    final[key].append(res[key])

            for key in resume:
                if not key in resume_final:
                    resume_final[key] = []
                    
                resume_final[key].append(resume[key])

            num += 1

        folder = basename(normpath(folder))
        m =re.match(r"{}(\d+)mx(\d+)m_(\d+)_(\d+)".format(name), folder)
        size = m.group(1)
        devices = m.group(3)
        tmi = int(m.group(4))

        print("Average from {} scalar output files".format(num))
        result = {}
        for key in final:
            result[key+"_var"] = np.var(final[key], ddof=1)
            result[key] = np.mean(final[key])
            result[key+"_conf"] = mean_confidence_interval(final[key])
            result[key+"_format"] = "${0:.3f} \pm {1:.3f}$".format(result[key], result[key+"_conf"])

        for key in resume_final:
            result[key] = np.mean(resume_final[key])
        
        processed_output = "{}x{}-{}.json".format(size, size, devices)

        data = {}
        if isfile(processed_output):
            with open(processed_output, 'r') as f:
                data = json.load(f)
                data['prr'].append([tmi, result['prr'], result['prr_conf']])
                data['prrack'].append([tmi, result['prrack'], result['prrack_conf']])
                data['prrreq'].append([tmi, result['prrreq'], result['prrreq_conf']])
                data['latency'].append([tmi, result['latency'], result['latency_conf']])
                data['acklatency'].append([tmi, result['acklatency'], result['acklatency_conf']])
                data['dissemination'].append([tmi, result['dissemination'], result['dissemination_conf']])
        else:
            data['prr'] = [[tmi, result['prr'], result['prr_conf']]]
            data['prrack'] = [[tmi, result['prrack'], result['prrack_conf']]]
            data['prrreq'] = [[tmi, result['prrreq'], result['prrreq_conf']]]
            data['latency'] = [[tmi, result['latency'], result['latency_conf']]]
            data['acklatency'] = [[tmi, result['acklatency'], result['acklatency_conf']]]
            data['dissemination'] = [[tmi, result['dissemination'], result['dissemination_conf']]]

        with open(processed_output, 'w') as f:
            json.dump(data, f, indent=4)

	with open("{}x{}-{}-{}-sum.json".format(size,size,devices,tmi), "w") as f:
            json.dump(result, f, indent=4, sort_keys=True)
	
