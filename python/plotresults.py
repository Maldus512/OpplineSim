import matplotlib.pyplot as plt
import numpy as np
import json
import glob
import re

TMI = [60, 540, 900]
RESULTS = ["prr", "prrreq", "prrack"]
RESULTS2 = ["latency", "acklatency"]
RESULTS3 = ["dissemination"]
COLORS = { "prr": "r", "prrreq": "b", "prrack": "y", "latency":"b", "acklatency":"y", "dissemination":"g"}


def plotResults(results, name, axis=None):
    files = glob.glob("[0-9]*x[0-9]*-[0-9]*.json")
    for jf in files:
        res = re.match(r"(\d+)x(\d+)-(\d+).json", jf)
        if not res:
            continue
        field = res.group(1)
        devices = res.group(3)
        with open(jf, "r") as f:
            plt.figure()
            data = json.load(f)
            for key in results:
                dom = []
                values = []
                conf_x = []
                conf = []
                for tup in data[key]:
                    dom.append(tup[0])
                    values.append(tup[1])
                    conf_x.append(tup[0])
                    conf_x.append(tup[0])
                    conf.append(tup[1]+tup[2])
                    conf.append(tup[1]-tup[2])

                plt.plot(dom, values, COLORS[key]+"-o", label=str(key))
                plt.scatter(conf_x, conf, color=COLORS[key], marker='+')
                plt.legend()
            
            if axis:
                plt.yticks(np.arange(0,1,.1))
                plt.axis(axis)
            
            plt.xticks(TMI)
            plt.xlabel("$T_{mi}$")
            plt.ylabel(name)
            plt.title("Field {} with {} devices".format(field, devices))

        plt.savefig("{}x{}_{}-{}.pdf".format(field,field, devices, name))

if __name__ == "__main__":
    plotResults(RESULTS, "prr",[0, 1000, 0, 1])
    plotResults(RESULTS2, "lat")
    plotResults(RESULTS3, "diss", [0,1000, 0,1])

#plt.plot([1,2,3,4], [2,3,2,4], "b-o")
#plt.ylabel("PRR")
#plt.show()
