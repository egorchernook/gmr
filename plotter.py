import matplotlib.pyplot as plt
import os

fig, ax = plt.subplots()
ax.tick_params(direction='in')
ax.set_xlabel(r't - t_w(mcs/s)')
ax.set_ylabel(r'$\delta$, \%')

with os.scandir(".") as it:
    for entry in it:
        if entry.is_file() and entry.name.find("GMR_tw=") != -1:
            tw = entry.name[entry.name.index(
                '=') + 1: entry.name.index('.')]
            file = open(entry.name, 'r')
            file.readline()

            gmr_lhc = list()
            gmr_lhc_err = list()
            gmr_uhc = list()
            gmr_uhc_err = list()

            counter: int = 0
            x_ax = list()
            for line in file:
                if line != '':
                    gmr_lhc_val, gmr_lhc_err_val, gmr_uhc_val, gmr_uhc_err_val = line.split(
                        '\t')
                    gmr_lhc.append(float(gmr_lhc_val))
                    gmr_lhc_err.append(float(gmr_lhc_err_val))
                    gmr_uhc.append(float(gmr_uhc_val))
                    gmr_uhc_err.append(float(gmr_uhc_err_val))

                    counter += 1
                    x_ax.append(counter)

            file.close()
            ax.errorbar(x_ax, gmr_lhc, yerr=gmr_lhc_err, label=tw)

ax.legend()
plt.savefig("GMR_lhc.pdf")

fig, ax = plt.subplots()
ax.tick_params(direction='in')
ax.set_xlabel(r't - t_w(mcs/s)')
ax.set_ylabel(r'$\delta$, \%')

with os.scandir(".") as it:
    for entry in it:
        if entry.is_file() and entry.name.find("GMR_tw=") != -1:
            tw = entry.name[entry.name.index(
                '=') + 1: entry.name.index('.')]
            file = open(entry.name, 'r')
            file.readline()

            gmr_lhc = list()
            gmr_lhc_err = list()
            gmr_uhc = list()
            gmr_uhc_err = list()

            counter: int = 0
            x_ax = list()
            for line in file:
                if line != '':
                    gmr_lhc_val, gmr_lhc_err_val, gmr_uhc_val, gmr_uhc_err_val = line.split(
                        '\t')
                    gmr_lhc.append(float(gmr_lhc_val))
                    gmr_lhc_err.append(float(gmr_lhc_err_val))
                    gmr_uhc.append(float(gmr_uhc_val))
                    gmr_uhc_err.append(float(gmr_uhc_err_val))

                    counter += 1
                    x_ax.append(counter)

            file.close()
            ax.errorbar(x_ax, gmr_uhc_val, yerr=gmr_uhc_err_val, label=tw)

ax.legend()

plt.savefig("GMR_uhc.pdf")
