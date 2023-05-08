import math
from typing import List
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
import os
import numpy as np


def get_vals(file):
    counter: int = 0
    x_ax = list()

    vals = list()
    for line in file:
        if line != '':
            str_line = line.split('\t')
            try:
                if str_line.index('\n') != -1:
                    str_line.pop(str_line.index('\n'))
            except ValueError:
                pass

            if not str_line:
                break

            vals_line = np.array(str_line, dtype=np.double)
            vals.append(vals_line)
            counter += 1
            x_ax.append(counter)
    return x_ax, vals


def plot_MR_t_impl(idx, name, text: str):
    fig, ax = plt.subplots()
    ax.tick_params(direction='in')
    ax.set_xlabel(r'$t - t_w$(mcs/s)')
    ax.set_ylabel(r'$\delta$, %')

    tws: List[int] = list()
    mr_vals: List[List[np.double]] = list()
    mr_vals_err: List[List[np.double]] = list()
    with os.scandir(".") as it:
        for entry in it:
            if entry.is_file() and entry.name.find("MR_tw=") != -1:
                tw = entry.name[entry.name.index(
                    '=') + 1: entry.name.index('.')]
                tws.append(tw)
                file = open(entry.name, 'r')
                file.readline()
                x_axis, vals = get_vals(file)
                y_axis: List[np.double] = list()
                y_axis_err: List[np.double] = list()

                for step in range(len(x_axis)):
                    y_axis.append(vals[step][idx])
                    y_axis_err.append(vals[step][idx + 1])

                mr_vals.append(y_axis)
                mr_vals_err.append(y_axis_err)
                file.close()
                ax.errorbar(x_axis, y_axis, yerr=y_axis_err,
                            label=r'$t - t_w$ = ' + tw, errorevery=len(x_axis) // 5, elinewidth=0.2, capsize=3, capthick=0.5)

    ax.legend()
    x_min, x_max = ax.xaxis.get_data_interval()
    y_min, y_max = ax.yaxis.get_data_interval()
    x_text = x_max - 2*(np.abs(x_max - x_min) / 10)
    y_text = y_min + 4*(np.abs(y_max - y_min) / 10)

    plt.text(x=x_text, y=y_text, s=text)
    plt.savefig(name + ".pdf")
    plt.close()
    min_tw = min(tws)
    min_idx = tws.index(min_tw)
    return mr_vals[min_idx], mr_vals_err[min_idx]


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


def plot_MR_t(path, text: str):
    init_dir = os.path.abspath(os.path.curdir)
    os.chdir(path)

    vals_lhc, vals_lhc_stat_err = plot_MR_t_impl(0, "MR_lhc", text)
    vals_uhc, vals_uhc_stat_err = plot_MR_t_impl(2, "MR_uhc", text)

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


def plot_func_t_impl(idx, text: str, name: str, output_file_name: str, axis_name: str = None):
    if axis_name == None:
        axis_name = name
    fig, ax = plt.subplots()
    ax.tick_params(direction='in')
    ax.set_xlabel(r'$t$(mcs/s)')
    ax.set_ylabel(axis_name)

    x_axis: List[np.double] = list()
    y_axis: List[np.double] = list()
    y_axis_err: List[np.double] = list()
    with os.scandir(".") as it:
        for entry in it:
            if entry.is_file() and entry.name.find(name) != -1 and entry.name.endswith('txt'):
                file = open(entry.name, 'r')
                try:
                    file.readline()
                except UnicodeDecodeError:
                    continue

                x_axis, vals = get_vals(file)

                for step in range(len(x_axis)):
                    y_axis.append(vals[step][idx])
                    y_axis_err.append(vals[step][idx + 1])

                file.close()
                ax.errorbar(x_axis.copy(), y_axis.copy(), yerr=y_axis_err.copy(), errorevery=len(
                    x_axis) // 5, elinewidth=0.2, capsize=3, capthick=0.5)

    x_min, x_max = ax.xaxis.get_data_interval()
    y_min, y_max = ax.yaxis.get_data_interval()
    x_text = x_max - 2*(np.abs(x_max - x_min) / 10)
    y_text = y_min + 4*(np.abs(y_max - y_min) / 10)

    plt.text(x=x_text, y=y_text, s=text)
    plt.savefig(output_file_name + ".pdf")
    plt.close()

    return x_axis, y_axis, y_axis_err


def plot_func_t(path, text: str, name: str, axis_name: str = None):
    init_dir = os.path.abspath(os.path.curdir)
    os.chdir(path)

    x1, y1, err1 = plot_func_t_impl(0, text, name, name + '_1', axis_name)
    x2, y2, err2 = plot_func_t_impl(2, text, name, name + '_2', axis_name)

    mean_y1 = np.mean(y1)
    err_y1 = np.std(y1)
    mean_err1 = np.mean(err1)
    err_err1 = np.std(err1)

    mean_y2 = np.mean(y2)
    err_y2 = np.std(y2)
    mean_err2 = np.mean(err2)
    err_err2 = np.std(err2)

    os.chdir(init_dir)

    return (mean_y1, err_y1, mean_err1, err_err1), (mean_y2, err_y2, mean_err2, err_err2)


def go_and_draw(dir, text: str):
    h_list: List[float] = list()
    m_list: List[np.ndarray[np.longdouble]] = list()
    j_list: List[np.ndarray[np.longdouble]] = list()
    Nup_list: List[np.ndarray[np.longdouble]] = list()
    Ndown_list: List[np.ndarray[np.longdouble]] = list()
    P_list: List[np.ndarray[np.longdouble]] = list()
    mr_lhc_list: List[np.ndarray[np.double]] = list()
    mr_uhc_list: List[np.ndarray[np.double]] = list()
    for dr in os.listdir(dir):
        abs_path = os.path.abspath(os.path.join(dir, dr))

        if os.path.isdir(abs_path):
            print('\tgo to path ' + abs_path)
            head, tail = os.path.split(abs_path)
            if tail.find('h = (') != -1:
                h = tail.split('(')[1].split(',')[0].strip()
                if h.strip() != "0":
                    lhc, uhc = plot_MR_t(abs_path, text)
                    mr_lhc_list.append(lhc)
                    mr_uhc_list.append(uhc)
                else:
                    mr_lhc_list.append([0.0, 0.0, 0.0, 0.0])
                    mr_uhc_list.append([0.0, 0.0, 0.0, 0.0])

                h_list.append(float(h))

                m_line = find_mean_m(abs_path)
                m_list.append(m_line)

                j_line = plot_func_t(abs_path, text, 'j', 'J')
                j_list.append(j_line)

                Nup_line = plot_func_t(
                    abs_path, text, 'Nup', r'$N_{\uparrow}$')
                Nup_list.append(Nup_line)

                Ndown_line = plot_func_t(
                    abs_path, text, 'Ndown', r'$N_{\downarrow}$')
                Ndown_list.append(Ndown_line)

                P_line = plot_func_t(abs_path, text, 'P', 'P')
                P_list.append(P_line)

            res_h, res_m, res_mr_lhc, res_mr_uhc = go_and_draw(abs_path, text)
            if len(h_list) == 0:
                h_list = res_h
            elif not len(res_h) == 0:
                h_list.append(res_h)

            if len(m_list) == 0:
                m_list = res_m
            elif not len(res_m) == 0:
                m_list.append(res_m)

            if len(mr_lhc_list) == 0:
                mr_lhc_list = res_mr_lhc
            elif not len(res_mr_lhc) == 0:
                mr_lhc_list.append(res_mr_lhc)

            if len(mr_uhc_list) == 0:
                mr_uhc_list = res_mr_uhc
            elif not len(res_mr_uhc) == 0:
                mr_uhc_list.append(res_mr_uhc)

    return (h_list, m_list, mr_lhc_list, mr_uhc_list)


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

                        text = N_str + T0_str + Ts_str
                        h_list, m_list, mr_lhc_list_full, mr_uhc_list_full = go_and_draw(
                            cur, text)

                        mr_lhc_list, mr_lhc_err_list = as_mean_and_err(
                            mr_lhc_list_full)
                        mr_uhc_list, mr_uhc_err_list = as_mean_and_err(
                            mr_uhc_list_full)

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

                        h_list5, mr_lhc_list = cosort(h_list5, mr_lhc_list)
                        h_list6, mr_lhc_err_list = cosort(
                            h_list6, mr_lhc_err_list)

                        h_list7, mr_uhc_list = cosort(h_list7, mr_uhc_list)
                        h_list8, mr_uhc_err_list = cosort(
                            h_list8, mr_uhc_err_list)

                        mr_list: List[np.double] = list()
                        mr_err_list: List[np.double] = list()
                        for idx in range(len(m_fst_list)):
                            if m_fst_list[idx] * m_snd_list[idx] > 0:
                                mr_list.append(mr_uhc_list[idx])
                                mr_err_list.append(mr_uhc_err_list[idx])
                            else:
                                mr_list.append(mr_lhc_list[idx])
                                mr_err_list.append(mr_lhc_err_list[idx])

                        pol_list: List[np.double] = list()
                        pol_err_list: List[np.double] = list()
                        pol_list.append(0.0)
                        pol_err_list.append(0.0)
                        for idx in range(1, len(mr_list)):
                            mr = mr_list[idx]
                            value = np.sqrt(mr / (200.0 + mr))  # mr in %
                            pol_list.append(value)
                            err = (
                                mr_err_list[idx] / mr + mr_err_list[idx] / (200.0 + mr)) / (2.0 * np.sqrt(value))
                            pol_err_list.append(err)

                        plot_with_h_as_x_ax([h_list1, h_list3], [m_fst_list, m_snd_list],
                                            [m_fst_list_err, m_snd_list_err],
                                            [r'$m_{x}^{1}$', r'$m_{x}^{2}$'],
                                            r'$h_{x}(J_{1})$', r'$m_{x}$',
                                            "m_h", text)

                        plot_with_h_as_x_ax([h_list5], [mr_list], [
                                            mr_err_list], [r'$\delta$'],
                                            r'$h_{x}(J_{1})$', r'$\delta ,\%$',
                                            "MR_h", text)

                        plot_with_h_as_x_ax([h_list8], [pol_list], [
                                            pol_err_list], [r'$P_s$'],
                                            r'$h_{x}(J_{1})$', r'$P_s$',
                                            "Ps_h", text)

                        os.chdir("..")
                os.chdir("..")
        os.chdir("..")
