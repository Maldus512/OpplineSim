from glob import glob
import sys
import os
import matplotlib.pyplot as plt
import re
import numpy as np

SLICE = 20
SIMTIME = 14400
slices = []

tmp = SIMTIME/SLICE
for i in range(SLICE):
    slices.append(tmp)
    tmp+= SIMTIME/SLICE

def processEvents(filename):
    events = {
        'received_request': [],
        'received_ack' : []
    }
    with open(filename, 'r') as f:
        for line in f.readlines():
            res = re.match(r"scalar\s+Field.hosts\[(\d+)\].app\s+#(received_request|received_ack)\s+(\d+\.\d+)", line)
            if res:
                events[res.group(2)].append(float(res.group(3)))

    statsreq = []
    statsack= []
    sindex = 0
    tot = 0
    for s in slices:
        num = 0
        while sindex < len(events['received_request']) and events['received_request'][sindex] <= s:
            num += 1
            sindex += 1
        
        tot += num
        statsreq.append(num)


    sindex = 0
    for s in slices:
        num = 0
        while sindex < len(events['received_ack']) and events['received_ack'][sindex] <= s:
            num += 1
            sindex += 1

        statsack.append(num)



    return statsreq, statsack


if __name__ == "__main__":
    if len(sys.argv) < 2:
        exit(1)

    files = glob(os.path.join(sys.argv[1], "*.sca"))

    requests = []
    acknowledgements = []

    for i in range(len(slices)):
        requests.append([])
        acknowledgements.append([])

    plt.axis([0,14400,0,20])
    plt.xlabel("tempo")
    plt.ylabel("numero di messaggi ricevuti")
    for f in files:
        req, ack = processEvents(f)

        plt.plot(slices, req, "b-o", label="requests")
        plt.plot(slices, ack, "y-o", label="acknowledgements")

        for i in range(len(req)):
            requests[i].append(req[i])

        for i in range(len(ack)):
            acknowledgements[i].append(ack[i])

    plt.figure()

    plt.axis([0,14400,0,20])
    plt.xlabel("tempo")
    plt.ylabel("numero di messaggi ricevuti")
    req_plot = [] 
    for r in requests:
        req_plot.append(np.mean(r))

    ack_plot = [] 
    for r in acknowledgements:
        ack_plot.append(np.mean(r))

    plt.plot(slices, req_plot, "b-o", label="requests")
    plt.plot(slices, ack_plot, "y-o", label="acknowledgements")
    plt.axvline(x=3600, color='g')
    plt.legend()
    plt.xlabel("tempo")
    plt.ylabel("numero di messaggi ricevuti")

    plt.savefig("{}.pdf".format(sys.argv[1]))
        

