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
            x_ax.append(counter)
            counter += 1

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


def plot_func_t_impl(text: str, name: str, output_file_name: str,  labels: List[str], axis_name: str = None):
    if axis_name == None:
        axis_name = name
    fig, ax = plt.subplots()
    ax.tick_params(direction='in', which='both')
    ax.minorticks_on()
    ax.set_xlabel(r'$t$(mcs/s)')
    ax.set_ylabel(axis_name)

    x_axises: List[List[np.double]] = list()
    y_axises: List[List[np.double]] = list()
    y_axises_errors: List[List[np.double]] = list()
    with os.scandir(".") as it:
        for entry in it:
            if entry.is_file() and entry.name.find(name) != -1 and entry.name.endswith('txt'):
                file = open(entry.name, 'r')
                try:
                    file.readline()
                except UnicodeDecodeError:
                    continue

                x_axis, vals = get_vals(file)
                file.close()
                x_axises.append(x_axis.copy())

                for idx in range(0, len(vals[0]) - 1, 2):
                    y_axis: List[np.double] = list()
                    y_axis_err: List[np.double] = list()
                    for step in range(len(x_axis)):
                        y_axis.append(vals[step][idx])
                        y_axis_err.append(vals[step][idx + 1])

                    y_axises.append(y_axis)
                    y_axises_errors.append(y_axis_err)

                    ax.errorbar(x_axis.copy()[50:], y_axis[50:], yerr=y_axis_err[50:], errorevery=len(
                        x_axis) // 5, label=labels[idx // 2], elinewidth=0.2, capsize=3, capthick=0.5)

    x_min, x_max = ax.xaxis.get_data_interval()
    y_min, y_max = ax.yaxis.get_data_interval()
    x_text = x_max - 2*(np.abs(x_max - x_min) / 10)
    y_text = y_min + 2*(np.abs(y_max - y_min) / 10)

    plt.legend()
    plt.text(x=x_text, y=y_text, s=text)
    plt.tight_layout()
    plt.savefig(output_file_name + ".pdf")
    plt.close()

    return x_axises, y_axises, y_axises_errors


def plot_func_t(path, text: str, name: str, axis_name: str = None, amount=2):
    init_dir = os.path.abspath(os.path.curdir)
    os.chdir(path)

    labels: List[str] = list()
    for i in range(amount):
        labels.append(name + str(i + 1))

    x, y, y_err = plot_func_t_impl(text, name, name, labels, axis_name)

    res: List[np.ndarray[np.double]] = list()
    for idx in range(amount):
        mean = np.mean(y[idx])
        err = np.std(y[idx])
        mean_err = np.mean(y_err[idx])
        err_err = np.std(y_err[idx])
        res.append(np.array([mean, err, mean_err, err_err]))

    os.chdir(init_dir)

    return res


def go_and_draw(dir, text: str):
    h_list: List[float] = list()
    m_list: List[np.ndarray[np.longdouble]] = list()
    j_list: List[List[np.ndarray[np.longdouble]]] = list()
    Nup_list: List[List[np.ndarray[np.longdouble]]] = list()
    Ndown_list: List[List[np.ndarray[np.longdouble]]] = list()
    P_list: List[List[np.ndarray[np.longdouble]]] = list()
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

                plot_func_t(abs_path, text, 'cos_theta', r'\theta', 1)
                plot_func_t(abs_path, text, 'cos_thetaXZ', r'\theta_{XZ}', 1)

                Nup_line = plot_func_t(
                    abs_path, text, 'Nup', r'$N_{\uparrow}$')
                Nup_list.append(Nup_line)

                Ndown_line = plot_func_t(
                    abs_path, text, 'Ndown', r'$N_{\downarrow}$')
                Ndown_list.append(Ndown_line)

                P_line = plot_func_t(abs_path, text, 'P', 'P')
                P_list.append(P_line)

            res_h, res_m, res_mr_lhc, res_mr_uhc, res_j, res_Nup, res_Ndown, res_P = go_and_draw(
                abs_path, text)
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

            if len(j_list) == 0:
                j_list = res_j
            elif not len(res_j) == 0:
                j_list.append(res_j)

            if len(Nup_list) == 0:
                Nup_list = res_Nup
            elif not len(res_Nup) == 0:
                Nup_list.append(res_Nup)

            if len(Ndown_list) == 0:
                Ndown_list = res_Ndown
            elif not len(res_Ndown) == 0:
                Ndown_list.append(res_Ndown)

            if len(P_list) == 0:
                P_list = res_P
            elif not len(res_P) == 0:
                P_list.append(res_P)

    return (h_list, m_list, mr_lhc_list, mr_uhc_list, j_list, Nup_list, Ndown_list, P_list)


def cosort(a, b):
    res = sorted(zip(a, b), key=lambda tup: tup[0], reverse=False)
    return [x[0] for x in res], [x[1] for x in res]


def plot_with_h_as_x_ax(x_axs: List, y_axs: List, yerrs: List, data_labels: List, x_label, y_label, name, text=None):
    fig, ax = plt.subplots()
    fig.subplots_adjust(top=0.95, right=0.95)
    ax.tick_params(direction='in', which='both')
    ax.set_xlabel(x_label)
    ax.set_ylabel(y_label)

    markers = ['s', 'o', 'v', '^', 'p', '<', 'P', '>', 'h', '*', 'd', 'x', '8']

    for i in range(len(x_axs)):
        ax.errorbar(x_axs[i], y_axs[i], yerr=yerrs[i], fmt=markers[i]+'-',
                    label=data_labels[i], elinewidth=0.2, capsize=3, capthick=0.5)

    ax.legend()
    x_min, x_max = ax.xaxis.get_data_interval()
    y_min, y_max = ax.yaxis.get_data_interval()
    x_tick_size = abs(x_max - x_min) / 10
    ax.xaxis.set_major_locator(
        ticker.MultipleLocator(x_tick_size))
    ax.xaxis.set_minor_locator(
        ticker.MultipleLocator(x_tick_size / 2))

    y_tick_size = abs(y_max - y_min) / 10
    ax.yaxis.set_major_locator(
        ticker.MultipleLocator(y_tick_size))
    ax.yaxis.set_minor_locator(
        ticker.MultipleLocator(y_tick_size / 2))

    x = x_max - 3*x_tick_size
    y = y_min + 4*y_tick_size

    plt.text(x=x, y=y, s=text)
    plt.tight_layout()
    plt.savefig(name + ".pdf", dpi=600)
    plt.close()


def as_mean_and_err(vals):
    mean: List[np.double] = list()
    err: List[np.double] = list()

    for line in vals:
        mean.append(line[0])
        err.append(line[1] + line[2] + line[3])
    return mean, err


print("Start")
for dir_N in os.listdir():
    print(f"Dir: {dir_N}")
    if dir_N.find("N = ") != -1:
        os.chdir(dir_N)
        N_str = "N = " + os.path.split(dir_N)[1].split(' ')[2].strip() + "\n"
        for dir_T0 in os.listdir():
            print(f"Dir: {dir_T0}")
            if dir_T0.find("T_creation = ") != -1:
                os.chdir(dir_T0)
                T0_str = r"$T_{creation}$ = " + \
                    os.path.split(dir_T0)[1].split(' ')[2].strip() + "\n"
                for dir_Ts in os.listdir():
                    print(f"Dir: {dir_Ts}")
                    if dir_Ts.find("T_sample = ") != -1:
                        os.chdir(dir_Ts)
                        Ts_str = r"$T_{sample}$ = " + \
                            os.path.split(dir_Ts)[1].split(
                                ' ')[2].strip() + "\n"
                        cur = os.curdir

                        text = N_str + T0_str + Ts_str
                        h_list, m_list, mr_lhc_list_full, mr_uhc_list_full, j_list, Nup_list, Ndown_list, P_list = go_and_draw(
                            cur, text)

                        print('\tdraw plots of functions depending on field')

                        mr_lhc_list, mr_lhc_err_list = as_mean_and_err(
                            mr_lhc_list_full)
                        mr_uhc_list, mr_uhc_err_list = as_mean_and_err(
                            mr_uhc_list_full)

                        m_plane_fst_list: List[np.longdouble] = list()
                        m_plane_fst_list_err: List[np.longdouble] = list()
                        m_plane_snd_list: List[np.longdouble] = list()
                        m_plane_snd_list_err: List[np.longdouble] = list()
                        m_fst_list: List[np.longdouble] = list()
                        m_fst_list_err: List[np.longdouble] = list()
                        m_snd_list: List[np.longdouble] = list()
                        m_snd_list_err: List[np.longdouble] = list()
                        for arr in m_list:
                            m_fst_list.append(arr[2])
                            m_fst_list_err.append(arr[3])

                            m_fst = np.sqrt(arr[2]**2 + arr[4]**2)
                            m_plane_fst_list.append(m_fst)
                            m_plane_fst_list_err.append(
                                abs(arr[2]*arr[3] + arr[4]*arr[5]) / m_fst)

                            m_snd = np.sqrt(arr[10]**2 + arr[12]**2)
                            m_plane_snd_list.append(m_snd)
                            m_plane_snd_list_err.append(
                                abs(arr[10]*arr[11] + arr[12]*arr[13]) / m_snd)

                            m_snd_list.append(arr[10])
                            m_snd_list_err.append(arr[11])

                        Nup_fst_list: List[np.double] = list()
                        Nup_fst_list_err: List[np.double] = list()
                        Nup_snd_list: List[np.double] = list()
                        Nup_snd_list_err: List[np.double] = list()
                        Ndown_fst_list: List[np.double] = list()
                        Ndown_fst_list_err: List[np.double] = list()
                        Ndown_snd_list: List[np.double] = list()
                        Ndown_snd_list_err: List[np.double] = list()

                        P_fst_list: List[np.double] = list()
                        P_fst_list_err: List[np.double] = list()
                        P_snd_list: List[np.double] = list()
                        P_snd_list_err: List[np.double] = list()

                        for idx in range(len(P_list)):
                            mean = P_list[idx][0][0]
                            err = P_list[idx][0][1] + \
                                P_list[idx][0][2] + P_list[idx][0][3]
                            P_fst_list.append(mean)
                            P_fst_list_err.append(err)

                        for idx in range(len(P_list)):
                            mean = P_list[idx][1][0]
                            err = P_list[idx][1][1] + \
                                P_list[idx][1][2] + P_list[idx][1][3]
                            P_snd_list.append(mean)
                            P_snd_list_err.append(err)

                        for idx in range(len(Nup_list)):
                            mean = Nup_list[idx][0][0]
                            err = Nup_list[idx][0][1] + \
                                Nup_list[idx][0][2] + Nup_list[idx][0][3]
                            Nup_fst_list.append(mean)
                            Nup_fst_list_err.append(err)

                        for idx in range(len(Nup_list)):
                            mean = Nup_list[idx][1][0]
                            err = Nup_list[idx][1][1] + \
                                Nup_list[idx][1][2] + Nup_list[idx][1][3]
                            Nup_snd_list.append(mean)
                            Nup_snd_list_err.append(err)

                        for idx in range(len(Ndown_list)):
                            mean = Ndown_list[idx][0][0]
                            err = Ndown_list[idx][0][1] + \
                                Ndown_list[idx][0][2] + Ndown_list[idx][0][3]
                            Ndown_fst_list.append(mean)
                            Ndown_fst_list_err.append(err)

                        for idx in range(len(Ndown_list)):
                            mean = Ndown_list[idx][1][0]
                            err = Ndown_list[idx][1][1] + \
                                Ndown_list[idx][1][2] + Ndown_list[idx][1][3]
                            Ndown_snd_list.append(mean)
                            Ndown_snd_list_err.append(err)

                        h_list1 = h_list
                        h_list2 = h_list
                        h_list3 = h_list
                        h_list4 = h_list
                        h_list5 = h_list
                        h_list6 = h_list
                        h_list7 = h_list
                        h_list8 = h_list
                        h_list9 = h_list
                        h_list10 = h_list
                        h_list11 = h_list
                        h_list12 = h_list
                        h_list13 = h_list
                        h_list14 = h_list
                        h_list15 = h_list
                        h_list16 = h_list
                        h_list17 = h_list
                        h_list18 = h_list
                        h_list19 = h_list
                        h_list20 = h_list
                        h_list21 = h_list
                        h_list22 = h_list
                        h_list23 = h_list
                        h_list24 = h_list
                        h_list25 = h_list

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

                        h_list9, Nup_fst_list = cosort(h_list9, Nup_fst_list)
                        h_list10, Nup_fst_list_err = cosort(
                            h_list10, Nup_fst_list_err)

                        h_list11, Nup_snd_list = cosort(h_list11, Nup_snd_list)
                        h_list12, Nup_snd_list_err = cosort(
                            h_list12, Nup_snd_list_err)

                        h_list13, Ndown_fst_list = cosort(
                            h_list13, Ndown_fst_list)
                        h_list14, Ndown_fst_list_err = cosort(
                            h_list14, Ndown_fst_list_err)

                        h_list15, Ndown_snd_list = cosort(
                            h_list15, Ndown_snd_list)
                        h_list16, Ndown_snd_list_err = cosort(
                            h_list16, Ndown_snd_list_err)

                        h_list17, P_fst_list = cosort(
                            h_list17, P_fst_list)
                        h_list18, P_fst_list_err = cosort(
                            h_list18, P_fst_list_err)

                        h_list19, P_snd_list = cosort(
                            h_list19, P_snd_list)
                        h_list20, P_snd_list_err = cosort(
                            h_list20, P_snd_list_err)

                        h_list22, m_plane_fst_list = cosort(
                            h_list22, m_plane_fst_list)
                        h_list23, m_plane_snd_list = cosort(
                            h_list23, m_plane_snd_list)
                        h_list24, m_plane_fst_list_err = cosort(
                            h_list24, m_plane_fst_list_err)
                        h_list25, m_plane_snd_list_err = cosort(
                            h_list25, m_plane_snd_list_err)

                        m_plane_sum_list: List[np.longdouble] = list()
                        m_plane_sum_list_err: List[np.longdouble] = list()
                        for idx in range(len(m_plane_fst_list)):
                            m_plane_sum_list.append(
                                (m_plane_fst_list[idx] + m_plane_snd_list[idx]) / 2)
                            m_plane_sum_list_err.append(
                                (m_plane_fst_list_err[idx] + m_plane_snd_list_err[idx]) / 2)

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
                                mr_err_list[idx] / mr + mr_err_list[idx] / (200.0 + mr)) * np.sqrt(value) / 2.0
                            pol_err_list.append(err)

                        MR_from_Ps: List[np.double] = list()
                        MR_from_Ps_err: List[np.double] = list()
                        for idx in range(len(P_fst_list)):
                            p1 = P_fst_list[idx]
                            p2 = P_snd_list[idx]
                            p1_err = P_fst_list_err[idx]
                            p2_err = P_snd_list_err[idx]

                            tmr = 2 * p1 * p2/(1 - p1 * p2)
                            err = ((p1_err / p1 + p2_err / p2) +
                                   (p1_err / p1 + p2_err / p2)) * tmr
                            MR_from_Ps.append(tmr * 100)
                            MR_from_Ps_err.append(abs(err) * 100)

                        plot_with_h_as_x_ax([h_list1, h_list3], [m_fst_list, m_snd_list],
                                            [m_fst_list_err, m_snd_list_err],
                                            [r'$m_{x}^{1}$', r'$m_{x}^{2}$'],
                                            r'$h_{x}(J_{1})$', r'$m_{x}$',
                                            "mx_h", text)

                        plot_with_h_as_x_ax([h_list5], [mr_list], [np.zeros(len(MR_from_Ps))], #[mr_err_list], 
                                            [r'$\delta$'],
                                            r'$h_{x}(J_{1})$', r'$\delta ,\%$',
                                            "MR_h", text)

                        plot_with_h_as_x_ax([h_list8], [pol_list], [np.zeros(len(pol_list))], #[pol_err_list], 
                                            [r'$P_s$'],
                                            r'$h_{x}(J_{1})$', r'$P_s$',
                                            "Ps_from_MR_h", text)

                        plot_with_h_as_x_ax([h_list9, h_list11, h_list13, h_list15],
                                            [Nup_fst_list, Nup_snd_list,
                                                Ndown_fst_list, Ndown_snd_list],
                                            [Nup_fst_list_err, Nup_snd_list_err,
                                                Ndown_fst_list_err, Ndown_snd_list_err],
                                            [r'$N_{\uparrow}^{1}$', r'$N_{\uparrow}^{2}$',
                                                r'$N_{\downarrow}^{1}$', r'$N_{\downarrow}^{2}$'],
                                            r'$h_{x}(J_{1})$', r'$N$',
                                            "N_h", text)

                        plot_with_h_as_x_ax([h_list17, h_list19],
                                            [P_fst_list, P_snd_list],
                                            [P_fst_list_err, P_snd_list_err],
                                            [r'$P_{s}^{1}$', r'$P_{s}^{2}$'],
                                            r'$h_{x}(J_{1})$', r'$P_{s}$',
                                            "Ps_h", text)

                        plot_with_h_as_x_ax([h_list21], [MR_from_Ps], [MR_from_Ps_err],
                                            [r'$\delta_{pol}$'], r'$h_{x}(J_{1})$',
                                            r'$\delta_{pol} ,\%$',
                                            "MR_from_polarization_h", text)

                        plot_with_h_as_x_ax([h_list22, h_list23], [m_plane_fst_list, m_plane_snd_list],
                                            [m_plane_fst_list_err, m_plane_snd_list_err], 
                                            [r'$m^{1}$', r'$m^{2}$'],
                                            r'$h_{x}(J_{1})$', r'$m$',
                                            "m_h", text)

                        plot_with_h_as_x_ax([h_list22], [m_plane_sum_list], [m_plane_sum_list_err],
                                            [r'$sum\_of\_m_{planes}^{1,2}$'],
                                            r'$h_{x}(J_{1})$', r'$sum\_of\_m_{planes}^{1,2}$',
                                            "P_from_m_planes_h", text)
                        os.chdir("..")
                os.chdir("..")
        os.chdir("..")
