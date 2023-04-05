import math
from typing import List
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
import os
import numpy as np


def get_vals(file):
    counter: int = 0
    x_ax = list()

    gmr_lhc = list()
    gmr_lhc_err = list()
    gmr_uhc = list()
    gmr_uhc_err = list()
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
    return x_ax, gmr_lhc, gmr_lhc_err, gmr_uhc, gmr_uhc_err


def plot_GMR_t_impl(idx, name):
    fig, ax = plt.subplots()
    ax.tick_params(direction='in')
    ax.set_xlabel(r'$t - t_w$(mcs/s)')
    ax.set_ylabel(r'$\delta$, %')

    gmr_vals: List[np.double] = list()
    gmr_vals_err: List[np.double] = list()
    tw_min = 2**15
    with os.scandir(".") as it:
        for entry in it:
            if entry.is_file() and entry.name.find("GMR_tw=") != -1:
                tw = entry.name[entry.name.index(
                    '=') + 1: entry.name.index('.')]
                file = open(entry.name, 'r')
                file.readline()
                vals = get_vals(file)
                if int(tw) < tw_min:
                    gmr_vals = vals[idx]
                    gmr_vals_err = vals[idx+1]
                file.close()
                ax.errorbar(vals[0], vals[idx], yerr=vals[idx+1],
                            label=r'$t - t_w$ = ' + tw, errorevery=len(vals[0]) // 5, elinewidth=0.2, capsize=3, capthick=0.5)

    ax.legend()
    plt.savefig(name + ".pdf")
    plt.close()
    return gmr_vals, gmr_vals_err


def find_plateau(vals, n_param=0.95):
    if n_param >= 1:
        raise ValueError("n_param [" + n_param + "] should be lesser then 1")
    mean = np.mean(vals)
    sq_sum = np.std(vals)**2 * len(vals) * (len(vals) - 1)
    size = len(vals)
    for idx in range(math.ceil(len(vals)*n_param)):
        val = vals[idx]
        val_sq_sum = (val - mean)**2
        if val_sq_sum < sq_sum:
            break
        else:
            mean = (mean*size - val)
            size -= 1
            mean /= size
            sq_sum -= val_sq_sum

    return mean, (sq_sum / (size * (size - 1)))**0.5, size


def plot_GMR_t(path):
    init_dir = os.path.abspath(os.path.curdir)
    os.chdir(path)

    vals_lhc, vals_lhc_stat_err = plot_GMR_t_impl(1, "MR_lhc")
    vals_uhc, vals_uhc_stat_err = plot_GMR_t_impl(3, "MR_uhc")

    mean_lhs, mean_lhs_mcs_err, size_mean_mcs_lhc = find_plateau(vals_lhc)
    mean_uhs, mean_uhs_mcs_err, size_mean_mcs_uhc = find_plateau(vals_uhc)

    mean_vals_lhc_stat_err = np.mean(
        vals_lhc_stat_err[len(vals_lhc_stat_err) - size_mean_mcs_lhc::])
    std_vals_lhc_err = np.std(
        vals_lhc_stat_err[len(vals_lhc_stat_err) - size_mean_mcs_lhc::])

    mean_vals_uhc_stat_err = np.mean(
        vals_uhc_stat_err[len(vals_uhc_stat_err) - size_mean_mcs_uhc::])
    std_vals_uhc_err = np.std(
        vals_uhc_stat_err[len(vals_uhc_stat_err) - size_mean_mcs_uhc::])

    os.chdir(init_dir)
    return ([mean_lhs, mean_lhs_mcs_err, mean_vals_lhc_stat_err, std_vals_lhc_err],
            [mean_uhs, mean_uhs_mcs_err, mean_vals_uhc_stat_err, std_vals_uhc_err])


def read_mean_m():
    with open("m.txt") as file:
        head = file.readline()
        count: int = 0
        mean = np.zeros((len(head.split('\t')) - 1), np.longdouble)
        for line in file:
            mean += np.fromstring(line, dtype=np.longdouble, sep='\t')
            count += 1
        mean /= count

    return mean


def find_mean_m(path):
    init_dir = os.path.abspath(os.path.curdir)
    os.chdir(path)

    mean = read_mean_m()

    os.chdir(init_dir)
    return mean


def go(dir):
    h_list: List[float] = list()
    m_list: List[np.ndarray[np.longdouble]] = list()
    gmr_lhc_list: List[np.ndarray[np.double]] = list()
    gmr_uhc_list: List[np.ndarray[np.double]] = list()
    for dr in os.listdir(dir):
        abs_path = os.path.abspath(os.path.join(dir, dr))

        if os.path.isdir(abs_path):
            print('\tgo to path ' + abs_path)
            head, tail = os.path.split(abs_path)
            if tail.find('h = (') != -1:
                h = tail.split('(')[1].split(',')[0].strip()
                if h.strip() != "0":
                    lhc, uhc = plot_GMR_t(abs_path)
                    gmr_lhc_list.append(lhc)
                    gmr_uhc_list.append(uhc)
                else:
                    gmr_lhc_list.append([0.0, 0.0, 0.0, 0.0])
                    gmr_uhc_list.append([0.0, 0.0, 0.0, 0.0])

                h_list.append(float(h))

                m_line = find_mean_m(abs_path)
                m_list.append(m_line)

            res_h, res_m, res_gmr_lhc, res_gmr_uhc = go(abs_path)
            if len(h_list) == 0:
                h_list = res_h
            elif not len(res_h) == 0:
                h_list.append(res_h)

            if len(m_list) == 0:
                m_list = res_m
            elif not len(res_m) == 0:
                m_list.append(res_m)

            if len(gmr_lhc_list) == 0:
                gmr_lhc_list = res_gmr_lhc
            elif not len(res_gmr_lhc) == 0:
                gmr_lhc_list.append(res_gmr_lhc)

            if len(gmr_uhc_list) == 0:
                gmr_uhc_list = res_gmr_uhc
            elif not len(res_gmr_uhc) == 0:
                gmr_uhc_list.append(res_gmr_uhc)

    return (h_list, m_list, gmr_lhc_list, gmr_uhc_list)


def cosort(a, b):
    res = sorted(zip(a, b), key=lambda tup: tup[0], reverse=False)
    return [x[0] for x in res], [x[1] for x in res]


def plot_with_h_as_x_ax(x_axs: List, y_axs: List, yerrs: List, data_labels: List, x_label, y_label, name, text=None):
    fig, ax = plt.subplots()
    fig.subplots_adjust(top=0.95, right=0.95)
    ax.tick_params(direction='in', which='both')
    ax.set_xlabel(x_label)
    ax.set_ylabel(y_label)

    x_tick_size = abs(np.max(x_axs) - np.min(x_axs)) / 10
    ax.xaxis.set_major_locator(
        ticker.MultipleLocator(x_tick_size))
    ax.xaxis.set_minor_locator(
        ticker.MultipleLocator(x_tick_size / 2))

    y_tick_size = max(abs(np.max(y_axs) - np.min(y_axs)), np.max(yerrs)) / 10
    ax.yaxis.set_major_locator(
        ticker.MultipleLocator(y_tick_size))
    ax.yaxis.set_minor_locator(
        ticker.MultipleLocator(y_tick_size / 2))

    for i in range(len(x_axs)):
        ax.errorbar(x_axs[i], y_axs[i], yerr=yerrs[i], fmt='s-',
                    label=data_labels[i], elinewidth=0.2, capsize=3, capthick=0.5)

    ax.legend()
    x = np.max(x_axs) - 3*x_tick_size
    y = np.min(y_axs) + 4*y_tick_size

    plt.text(x=x, y=y, s=text)
    plt.savefig(name + ".pdf", dpi=600)
    plt.close()


def as_mean_and_err(vals):
    mean: List[np.double] = list()
    err: List[np.double] = list()

    for line in vals:
        mean.append(line[0])
        err.append(line[1] + line[2] + line[3])
    return mean, err


for dir_N in os.listdir():

    if dir_N.find("N = ") != -1:
        os.chdir(dir_N)
        N_str = "N = " + os.path.split(dir_N)[1].split(' ')[2].strip() + "\n"
        for dir_T0 in os.listdir():

            if dir_T0.find("T_creation = ") != -1:
                os.chdir(dir_T0)
                T0_str = r"$T_{creation}$ = " + \
                    os.path.split(dir_T0)[1].split(' ')[2].strip() + "\n"
                for dir_Ts in os.listdir():

                    if dir_Ts.find("T_sample = ") != -1:
                        os.chdir(dir_Ts)
                        Ts_str = r"$T_{sample}$ = " + \
                            os.path.split(dir_Ts)[1].split(
                                ' ')[2].strip() + "\n"
                        cur = os.curdir

                        h_list, m_list, gmr_lhc_list_full, gmr_uhc_list_full = go(
                            cur)

                        gmr_lhc_list, gmr_lhc_err_list = as_mean_and_err(
                            gmr_lhc_list_full)
                        gmr_uhc_list, gmr_uhc_err_list = as_mean_and_err(
                            gmr_uhc_list_full)

                        m_fst_list: List[np.longdouble] = list()
                        m_fst_list_err: List[np.longdouble] = list()
                        m_snd_list: List[np.longdouble] = list()
                        m_snd_list_err: List[np.longdouble] = list()
                        for arr in m_list:
                            m_fst_list.append(arr[2])
                            m_fst_list_err.append(arr[3])
                            m_snd_list.append(arr[10])
                            m_snd_list_err.append(arr[11])

                        h_list1 = h_list
                        h_list2 = h_list
                        h_list3 = h_list
                        h_list4 = h_list
                        h_list5 = h_list
                        h_list6 = h_list
                        h_list7 = h_list
                        h_list8 = h_list

                        h_list1, m_fst_list = cosort(h_list1, m_fst_list)
                        h_list2, m_fst_list_err = cosort(
                            h_list2, m_fst_list_err)

                        h_list3, m_snd_list = cosort(h_list3, m_snd_list)
                        h_list4, m_snd_list_err = cosort(
                            h_list4, m_snd_list_err)

                        h_list5, gmr_lhc_list = cosort(h_list5, gmr_lhc_list)
                        h_list6, gmr_lhc_err_list = cosort(
                            h_list6, gmr_lhc_err_list)

                        h_list7, gmr_uhc_list = cosort(h_list7, gmr_uhc_list)
                        h_list8, gmr_uhc_err_list = cosort(
                            h_list8, gmr_uhc_err_list)

                        gmr_list: List[np.double] = list()
                        gmr_err_list: List[np.double] = list()
                        for idx in range(len(m_fst_list)):
                            if m_fst_list[idx] * m_snd_list[idx] > 0:
                                gmr_list.append(gmr_uhc_list[idx])
                                gmr_err_list.append(gmr_uhc_err_list[idx])
                            else:
                                gmr_list.append(gmr_lhc_list[idx])
                                gmr_err_list.append(gmr_lhc_err_list[idx])

                        pol_list: List[np.double] = list()
                        pol_err_list: List[np.double] = list()
                        pol_list.append(0.0)
                        pol_err_list.append(0.0)
                        for idx in range(1, len(gmr_list)):
                            gmr = gmr_list[idx]
                            value = np.sqrt(gmr / (200.0 + gmr))  # gmr is a %
                            pol_list.append(value)
                            # ( gmr_err_list[idx] / gmr + gmr_err_list[idx] / (200.0 + gmr)) / (2.0 * np.sqrt(value))
                            err = 0.0
                            pol_err_list.append(err)

                        plot_with_h_as_x_ax([h_list1, h_list3], [m_fst_list, m_snd_list],
                                            [m_fst_list_err, m_snd_list_err],
                                            [r'$m_{x}^{1}$', r'$m_{x}^{2}$'],
                                            r'$h_{x}(J_{1})$', r'$m_{x}$',
                                            "m_h",
                                            N_str + T0_str + Ts_str)

                        plot_with_h_as_x_ax([h_list5], [gmr_list], [
                                            gmr_err_list], [r'$\delta$'],
                                            r'$h_{x}(J_{1})$', r'$\delta ,\%$',
                                            "MR_h",
                                            N_str + T0_str + Ts_str)

                        plot_with_h_as_x_ax([h_list8], [pol_list], [
                                            pol_err_list], [r'$P_s$'],
                                            r'$h_{x}(J_{1})$', r'$P_s$',
                                            "Ps_h",
                                            N_str + T0_str + Ts_str)

                        os.chdir("..")
                os.chdir("..")
        os.chdir("..")
