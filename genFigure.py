import csv
import statistics
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import numpy as np
import math
import os 
from config import *
from exp_config import *

# FileName = "sample_outputs/data-n_clients-chronos.csv"
# FileName = "sample_outputs/data-tpca-chronos.csv"
# FileName = "sample_outputs/data-crt_ratio-chronos.csv"
# # can be 90, 95, 99
# FileName = "sample_outputs/data-n_clients-slog.csv"
# FileName = "sample_outputs/data-tpca-slog.csv"
# FileName = "sample_outputs/data-crt_ratio-slog.csv"

# FileName = "sample_outputs/data-n_clients-brq.csv"
# # FileName = "sample_outputs/data-tpca-slog.csv"
# FileName = "sample_outputs/data-crt_ratio-slog.csv"

DataFilePrefix = "results/data"

SysNames = ['chronos', 'brq', 'slog', 'tapir']
FigNames = ['throughput', 'irt_med', 'irt_tail', 'crt_med', 'crt_tail']

def drawFig(trial_name):
    all_results = {}
    
    x_label_value = None

    for sysname in SysNames:
        dataFile = DataFilePrefix + "-" + trial_name + "-" + sysname + ".csv"
        if os.path.isfile(dataFile):
            print("Found data file{}".format(dataFile))
            rows = {}
            with open (dataFile, 'r') as f:
                reader = csv.reader(f)
                for row in reader:
                    if row[1] != "tput":
                    # this is a data column
                    # sort by the x_value
                    # 
                        x_value = float(row[0])
                        if not x_value in rows:
                            rows[x_value] = []
                        rows[x_value].append(row[1:])
                    else:
                        x_label_value = str(row[0])
            results = []
            for x_value in sorted(rows.keys()):
                data_point = []
                data = rows[x_value]

                count = len(data)
                print("{} results for x_value {}".format(count, x_value))
    
                tputs = []
                irt_meds = []
                irt_tails = []
                crt_meds = []
                crt_tails = []

                for d in data:
                    tputs.append(math.floor(float(d[0])))
                    irt_meds.append(math.floor(float(d[1])))
                    crt_meds.append(math.floor(float(d[5])))
                    
                    if TAIL_PERCENTILE == 90:
                        irt_tails.append(math.floor(float(d[2])))
                        crt_tails.append(math.floor(float(d[6])))
                    elif TAIL_PERCENTILE == 95:
                        irt_tails.append(math.floor(float(d[3])))
                        crt_tails.append(math.floor(float(d[7])))
                    elif TAIL_PERCENTILE == 99:
                        irt_tails.append(math.floor(float(d[3])))
                        crt_tails.append(math.floor(float(d[7])))
                    else:
                        print("Tail percentile can only be 90, 95, or 99!")
                        exit(-1)
                
                data_point.append(x_value)
                data_point.append(statistics.median(tputs))
                data_point.append(statistics.median(irt_meds))
                data_point.append(statistics.median(irt_tails))
                data_point.append(statistics.median(crt_meds))
                data_point.append(statistics.median(crt_tails))
                results.append(data_point)
            print(results)

            data = np.array(results)
            data = np.transpose(data)   
            all_results[sysname] = data

    fig, axs = plt.subplots(len(FigNames), figsize=(10, 50))
    fig.suptitle(trial_name)
    for i in range(len(FigNames)):
        x = all_results['chronos'][0]

        lines = []
        legends = []
        if 'chronos' in all_results:
            line = axs[i].plot(x, all_results['chronos'][i+1], color=COLOR_XXX, linestyle=LINE_XXX, marker=MARKER_XXX, markersize=MARKER_SIZE) #, marker=MARKER_SLAM)
            lines.append(line[0])
            legends.append('DAST')

        if 'brq' in all_results:
            line = axs[i].plot(x, all_results['brq'][i+1], color=COLOR_JANUS, linestyle=LINE_JANUS, marker=MARKER_JANUS, markersize=MARKER_SIZE) # , marker=MARKER_JANUS)
            lines.append(line[0])
            legends.append('Janus')

        if 'slog' in all_results:
            line = axs[i].plot(x, all_results['slog'][i+1] , color=COLOR_SLOG, linestyle=LINE_SLOG, marker=MARKER_SLOG, markersize=MARKER_SIZE) # , marker=MARKER_GOSSIP)
            lines.append(line[0])
            legends.append('SLOG')

        if 'tapir' in all_results:
            line = axs[i].plot(x, all_results['tapir'][i+1], color=COLOR_TAPIR, linestyle=LINE_TAPIR, marker=MARKER_TAPIR, markersize=MARKER_SIZE) # , marker=MARKER_FLOODING)
            lines.append(line[0])
            legends.append('Tapir')
        
        axs[i].set_xlabel(x_label_value)
        axs[i].set_ylabel(FigNames[i])

        if FigNames[i] == 'irt_med':
            axs[i].set_ylim((0, 200))
        if FigNames[i] == 'irt_tail':
            axs[i].set_ylim((0, 500))
        if FigNames[i] == 'crt_med':
            axs[i].set_ylim((0, 500))
        if FigNames[i] == 'crt_tail':
            axs[i].set_ylim((0, 1000))

    plt.tight_layout()

    name = DataFilePrefix + "-" + trial_name + ".pdf"
    plt.savefig(name)

        
        
        
        
        # for sysname, data in all_results.items():
        #     print(sysname, all_results[sysname][i+1]) 

             

def main():
    drawFig("crt_ratio")

if __name__ == "__main__":
    main()


# rows = {}
# with open (FileName, 'r') as f:
#     reader = csv.reader(f)
#     for row in reader:
#         if row[1] != "tput":
#             # this is a data column
#             # sort by the x_value
#             # 
#             x_value = float(row[0])
#             if not x_value in rows:
#                 rows[x_value] = []
#             rows[x_value].append(row[1:])

# results = []
# for x_value in sorted(rows.keys()):
#     data_point = []
#     data = rows[x_value]

#     count = len(data)
#     print("{} results for x_value {}".format(count, x_value))
#     #tput
#     tputs = []
#     irt_meds = []
#     irt_tails = []
#     crt_meds = []
#     crt_tails = []

#     for d in data:
#         tputs.append(math.floor(float(d[0])))
#         irt_meds.append(math.floor(float(d[1])))
#         crt_meds.append(math.floor(float(d[5])))
        
#         if TAIL_PERCENTILE == 90:
#             irt_tails.append(math.floor(float(d[2])))
#             crt_tails.append(math.floor(float(d[6])))
#         elif TAIL_PERCENTILE == 95:
#             irt_tails.append(math.floor(float(d[3])))
#             crt_tails.append(math.floor(float(d[7])))
#         elif TAIL_PERCENTILE == 99:
#             irt_tails.append(math.floor(float(d[3])))
#             crt_tails.append(math.floor(float(d[7])))
#         else:
#             print("Tail percentile can only be 90, 95, or 99!")
#             exit(-1)
    
#     data_point.append(x_value)
#     data_point.append(statistics.median(tputs))
#     data_point.append(statistics.median(irt_meds))
#     data_point.append(statistics.median(irt_tails))
#     data_point.append(statistics.median(crt_meds))
#     data_point.append(statistics.median(crt_tails))
#     results.append(data_point)
# print(results)

# data = np.array(results)
# data = np.transpose(data)


# for line in data:
#     print(line)