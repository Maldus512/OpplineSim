import re
import sys
import json
from glob import glob


def processStats(filename):
    stats = {}
    nlat = 0
    acknlat = 0
    numHost = 0

    stats['latency'] = 0
    stats['acklatency'] = 0
    stats['received'] = 0
    stats['ackreceived'] = 0
    stats['sent'] = 0
    stats['acksent'] = 0
    stats['acked'] = 0
    stats['generated'] = 0

    with open(filename, 'r') as f:
        for line in f.readlines():
            res = re.match(r"scalar\s+Field.hosts\[(\d+)\].app\s+#(latency|acklatency|received|ackreceived|sent|acksent|acked|generated)\s+(\d+)", line)
            if res:
                if int(res.group(1)) > numHost:
                    numHost = int(res.group(1))

                if res.group(2) == 'latency':
                    nlat += 1
                elif res.group(2) == 'acklatency':
                    acknlat += 1

                stats[res.group(2)] += int(res.group(3))

    try:
        prr = (stats['received']+stats['ackreceived'])/float(stats['sent']+stats['acksent'])
        prrreq = stats['received']/float(stats['sent'])
        prrack = stats['ackreceived']/float(stats['acksent'])
        latency = stats['latency']/float(nlat)
        acklatency = stats['acklatency']/float(acknlat)
        dissemination = stats['generated']/float((stats['sent'] + stats['acksent']) * numHost)
    except ZeroDivisionError:
        print("WARNING: some values are not valid")
        return {}

    results = {}
    results['prr'] = prr
    results['prrreq'] = prrreq
    results['prrack'] = prrack
    results['latency'] = latency
    results['acklatency'] = acklatency
    results['dissemination'] = dissemination
    #print(json.dumps(stats,indent=4))
    #print(json.dumps(results, indent=4))
    return results



if __name__ == '__main__':
    if len(sys.argv) < 2:
        exit(1)

    num = 0
    final = {}
    for filen in glob(sys.argv[1]):
        print("processing {} ...".format(filen))
        num += 1
        res = processStats(filen)
        for key in res:
            if not key in final:
                final[key] = 0
                
            final[key] += res[key]

    print("Average from {} scalar output files".format(num))
    for key in final:
        final[key] = final[key]/num
    
    print(json.dumps(final, indent=4))