from math import sqrt
from dataclasses import dataclass
from typing import Dict, List
from numpy.typing import ArrayLike
import matplotlib.pyplot as plt
import os
import numpy as np


def parse_path(path: str) -> List[str]:
    return path.split("/")[1:]


@dataclass
class Config:
    N: int
    T_creation: float
    T_sample: float

    def __key(self):
        return (self.N, self.T_creation, self.T_sample)

    def __hash__(self):
        return hash(self.__key())

    def __eq__(self, other):
        if isinstance(other, Config):
            return self.__key() == other.__key()
        return NotImplemented

    @staticmethod
    def parse(path: str) -> 'Config':
        parts = parse_path(path)
        N = int(parts[0].split("=")[1].strip())
        T_0 = float(parts[1].split("=")[1].strip())
        T_s = float(parts[2].split("=")[1].strip())
        return Config(N=N, T_creation=T_0, T_sample=T_s)

    def text(self) -> str:
        Tc = f"{int(round(self.T_creation*318.8, -2))}K"
        Ts = f"{int(round(self.T_sample*318.8, -2))}K"
        return f'N = {self.N}\nT_creation = {Tc}\nT_sample = {Ts}'

    def path(self) -> str:
        return f'N = {self.N}/T_creation = {self.T_creation}/T_sample = {self.T_sample}'


@dataclass(unsafe_hash=True)
class Field:
    x: float
    y: float
    z: float

    @staticmethod
    def parse(path: str) -> 'Field':
        parts = parse_path(path)
        vals = parts[3].split("=")[1].replace("(","").replace(")","").strip().split(",")
        x, y, z = [float(val.strip()) for val in vals]
        return Field(x=x, y=y, z=z)


def get_header(*, path: str, file: str) -> str:
    full_filename = os.path.join(path, file)

    header = ""
    with open(full_filename, "r") as f:
        header = f.readline()
    
    return header


def process_file(*, path: str, file: str):
    full_filename = os.path.join(path, file)

    header = ""
    with open(full_filename, "r") as f:
        header = f.readline()
    heads = []
    for head in header.split("\t"):
        heads.append(head.strip())

    conv = lambda x: x.strip()
    loaded = np.loadtxt(fname=full_filename, skiprows=1, converters=conv)
    data = np.flipud(np.rot90(loaded))
    x = np.arange(data.shape[1])

    np.savetxt(fname=full_filename.replace(".txt", ".dat"), comments="",
                header=("t\t" + header.replace("\t\t", "\terr\t")), 
                newline="\n", delimiter="\t", fmt=("%u\t" + "%1.2e\t" * data.shape[0]),
                X=np.concatenate((x.reshape(1, loaded.shape[0]).T, loaded), axis=1))

    mean = np.mean(data[..., 50:], axis=1).reshape((data.shape[0] // 2, 2))
    std = np.std(data[..., 50:], axis=1).reshape((data.shape[0] // 2, 2))
    err = np.sum(std, axis=1)
    mean_err = np.take(mean, 1, axis=1)
    for idx in range(len(mean_err)):
        np.put(mean, idx * mean.shape[1] + 1, err[idx] + mean_err[idx])

    fig, ax = plt.subplots()
    ax.tick_params(direction='in', which='both')
    ax.minorticks_on()
    ax.set_xlabel(r'$t$(mcs/s)')
    ax.set_ylabel(name)
    for idx in range(0, data.shape[0], 2):
        y = data[idx][50:]
        yerr = data[idx + 1][50:]
        ax.errorbar(x=x[50:], y=y, yerr=yerr,
            label=heads[idx], errorevery=len(x) // 5, elinewidth=0.2, capsize=3, capthick=0.5)

    ax.legend()
    x_min, x_max = ax.xaxis.get_data_interval()
    y_min, y_max = ax.yaxis.get_data_interval()
    x_tick_size = abs(x_max - x_min) / 10
    y_tick_size = abs(y_max - y_min) / 10

    plt.text(x=x_max - 3*x_tick_size, y=y_min + 4*y_tick_size, s=config.text())
    plt.tight_layout()
    plt.savefig(full_filename.replace(".txt", ".pdf"), dpi=600)
    plt.close()

    return mean


markers: List[str] = ['s', 'o', 'v', '^', 'p', '<', 'P', '>', 'h', '*', 'd', 'x', '8']
def plot(ax: plt.Axes, *, x: ArrayLike, y: ArrayLike, yerr: ArrayLike, fmt: str, label) -> None:
    ax.errorbar(x=x, y=y, yerr=yerr, fmt=fmt,
        label=label, elinewidth=0.2, capsize=3, capthick=0.5)


def prettify_labels(labels: List[str]) -> List[str]:
    ret = list()
    for label in labels:
        if label.count("_") == 0:
            ret.append(label)
            continue

        if label == "MR_h_lower_hc":
            ret.append(r"$\delta_h^{lhc}$")
            continue
        if label == "MR_h_upper_hc":
            ret.append(r"$\delta_h^{uhc}$")
            continue

        components = label.split("_")
        if len(components) == 2:
            ret.append(fr"${components[0]}_{components[1]}$")
        else:
            ret.append(fr"${components[0]}_\{components[1]}arrow^{components[2]}$")

    return ret


plt.rcParams["font.family"] = "Times New Roman"
plt.rcParams.update({'font.size': 14})
if __name__ == "__main__":
    last_config = None
    last_field = None


    datas: Dict[Config, Dict[str, Dict[Field, np.ndarray]]] = {}
    name_to_label: Dict[str, List[str]] = {}

    for path, dirs, files in os.walk("."):
        for file in files:
            name, extension = os.path.splitext(file)
            if extension != ".txt":
                continue

            config = Config.parse(path)
            field = Field.parse(path)
            if last_config == None or last_config != config:
                print(config)
            if last_field == None or last_field != field:
                print("\t" + str(field))
            last_config = config
            last_field = field
            print(f"\t\tProcessing file {os.path.join(path, file)}")

            try:
                average = process_file(path=path, file=file)
            except Exception as ex:
                print(f"{type(ex)} {ex} raised : skip file")
                continue

            header = get_header(path=path, file=file)
            name_to_label[name] = prettify_labels(header.split())

            if config not in datas:
                datas[config] = {name: {field: average}}
            else:
                if name not in datas[config]:
                    datas[config][name] = {field: average}
                else:
                    datas[config][name][field] = average

    print("\tCreating field plots...")
    for (config, data) in datas.items():
        critical_field = 0.0
        for (name, val) in data.items():
            print(f"\t\tWorking on {name}-h plot")
            x = list()
            y_list = np.zeros(shape=(len(name_to_label[name]), len(val)))
            yerr_list = np.zeros(shape=y_list.shape)
            field_counter = 0
            for (field, average) in val.items():
                x.append(field.x)
                for idx in range(average.shape[0]):
                    if name == "P" or name == "P_mod":
                        y_list[idx][field_counter] = np.absolute(average[idx][0]) * 100
                        yerr_list[idx][field_counter] = np.absolute(average[idx][1]) * 100
                    else:
                        y_list[idx][field_counter] = average[idx][0]
                        yerr_list[idx][field_counter] = average[idx][1]
                field_counter += 1

            fig, ax = plt.subplots()
            ax.tick_params(direction='in', which='both')
            ax.minorticks_on()
            ax.set_xlabel(r'$h_x$($J_1 \cdot \mu_{B}$)')
            
            if name == "P":
                ax.set_ylabel(r"$P_s$, %")
            elif name == "P_mod":
                ax.set_ylabel(r"$P^R_s$, %")
            elif name == "Nup":
                ax.set_ylabel(r"$N_\uparrow$, %")
            elif name == "Ndown":
                ax.set_ylabel(r"$N_\downarrow$, %")
            else:
                ax.set_ylabel(name)

            file_header = "t"
            file_data = np.zeros(shape=(1, y_list.shape[1]))
            file_data[0] = x
            for idx in range(y_list.shape[0]):
                if name == "m":
                    if name_to_label[name][idx].count("x"):
                        plot(ax, x=x, y=y_list[idx], yerr=yerr_list[idx], 
                            fmt=markers[idx]+'-', label=name_to_label[name][idx])
                        if name_to_label[name][idx] == "m2x":
                            for jdx in range(len(y_list[idx])):
                                if y_list[idx][jdx] > 0.0 and critical_field == 0.0:
                                    critical_field = x[jdx]
                                    break
                if config.N == 7 and name == "P_mod":
                    plot(ax, x=x, y=y_list[idx], yerr=np.zeros(shape=(yerr_list[idx].shape)),
                            fmt=markers[idx]+'-', label=name_to_label[name][idx])
                else:
                    plot(ax, x=x, y=y_list[idx], yerr=yerr_list[idx], 
                         fmt=markers[idx]+'-', label=name_to_label[name][idx])

                file_header += f"\t{name_to_label[name][idx]}\t\terr\t"
                file_data = np.concatenate(
                    (file_data, np.array([y_list[idx]]), np.array(object=[yerr_list[idx]])), 
                    axis=0)

            ax.legend()
            x_min, x_max = ax.xaxis.get_data_interval()
            y_min, y_max = ax.yaxis.get_data_interval()
            x_tick_size = abs(x_max - x_min) / 10
            y_tick_size = abs(y_max - y_min) / 10

            text = ""
            if name == "P_mod":
                text = config.text() + "\n" + r"$A_{fb}=-0.8$"
            else:
                text = config.text()

            plt.text(x=x_max - 3*x_tick_size, y=y_min + 4*y_tick_size, s=text)
            plt.tight_layout()
            plt.savefig(f"{config.path()}/{name}_h.pdf", dpi=600)
            plt.close()

            np.savetxt(fname=f"{config.path()}/{name}_h.dat", comments="",
                header=file_header,
                newline="\n", delimiter="\t", fmt=("%.2f\t" + "%1.2e\t" * (file_data.shape[0] - 1)),
                X=np.rot90(file_data))

        print(f"\t\tcritical field: {critical_field}")

        x = list()
        MR_R = list()
        MR_R_err = list()
        for (field, average) in data["MR_tw=0"].items():
            x.append(field.x)
            if field.x < critical_field:
                MR_R.append(average[0][0])
                MR_R_err.append(average[0][1])
            else:
                MR_R.append(average[1][0])
                MR_R_err.append(average[1][1])

        try:
            fig, ax = plt.subplots()
            ax.tick_params(direction='in', which='both')
            ax.minorticks_on()
            ax.set_xlabel(r'$h_x$($J_1$)')
            ax.set_ylabel(r'$\delta_{ТМС}, \%$')

            file_header = "t\t\tMR\t\terr\t\t"
            file_data = np.zeros(shape=(1, len(MR_R)))
            file_data[0] = x
            plot(ax, x=x, y=MR_R, yerr=MR_R_err, fmt=markers[0]+'-', label=r'$\delta_{R}$')

            file_data = np.concatenate(
                (file_data, np.array([MR_R]), np.array(object=[MR_R_err])), 
                    axis=0)

            ax.legend()
            x_min, x_max = ax.xaxis.get_data_interval()
            y_min, y_max = ax.yaxis.get_data_interval()
            x_tick_size = abs(x_max - x_min) / 10
            y_tick_size = abs(y_max - y_min) / 10

            plt.text(x=x_max - 3*x_tick_size, y=y_min + 4*y_tick_size, s=config.text())
            plt.tight_layout()
            plt.savefig(f"{config.path()}/MR_R_h.pdf", dpi=600)
            plt.close()

            np.savetxt(fname=f"{config.path()}/MR_R_h.dat", comments="",
                header=file_header,
                newline="\n", delimiter="\t", fmt=("%.2f\t" + "%.2f\t" * (file_data.shape[0] - 1)),
                X=np.rot90(file_data))
        except Exception as e:
            print(f"{type(e)} {e}")
        
        try:
            fig, ax = plt.subplots()
            ax.tick_params(direction='in', which='both')
            ax.minorticks_on()
            ax.set_xlabel(r'$h_x$($J_1$)')
            ax.set_ylabel(r'$P_{s}, \%$')

            file_header = "t\t\tPs\t\terr\t\t"
            file_data = np.zeros(shape=(1, len(MR_R)))
            file_data[0] = x
            Ps_R = list()
            Ps_R_err = list()
            for idx in range(len(MR_R)):
                mr = MR_R[idx]
                value = sqrt(mr / (200.0 + mr))  # mr in %
                Ps_R.append(abs(value)*100)
                err = (
                    MR_R_err[idx] / mr + MR_R_err[idx] / (200.0 + mr)) * sqrt(value) / 2.0
                Ps_R_err.append(err)
            plot(ax, x=x, y=Ps_R, yerr=Ps_R_err, fmt=markers[0]+'-', label=r'$P_s^R$')

            file_data = np.concatenate(
                (file_data, np.array([Ps_R]), np.array(object=[Ps_R_err])), 
                    axis=0)

            ax.legend()
            x_min, x_max = ax.xaxis.get_data_interval()
            y_min, y_max = ax.yaxis.get_data_interval()
            x_tick_size = abs(x_max - x_min) / 10
            y_tick_size = abs(y_max - y_min) / 10

            plt.text(x=x_max - 3*x_tick_size, y=y_min + 4*y_tick_size, s=config.text())
            plt.tight_layout()
            plt.savefig(f"{config.path()}/Ps_R_h.pdf", dpi=600)
            plt.close()

            np.savetxt(fname=f"{config.path()}/Ps_R_h.dat", comments="",
                header=file_header,
                newline="\n", delimiter="\t", fmt=("%.2f\t" + "%.2f\t" * (file_data.shape[0] - 1)),
                X=np.rot90(file_data))
        except Exception as e:
            print(f"{type(e)} {e}")

        x = list()
        MR_Ps = list()
        MR_Ps_err = list()
        for (field, average) in data["P_mod"].items():
            x.append(field.x)

            p1 = average[0][0]
            p1_err = average[0][1]
            p2 = average[1][0]
            p2_err = average[1][1]

            tmr = abs(2 * p1 * p2/(1 - p1 * p2))
            err = abs((p1_err / p1 + p2_err / p2) +
                (p1_err / p1 + p2_err / p2)) * tmr

            MR_Ps.append(tmr * 100)
            if config.N == 7:
                MR_Ps_err.append(0.0)
            else:
                MR_Ps_err.append(err * 100)

        try:
            fig, ax = plt.subplots()
            ax.tick_params(direction='in', which='both')
            ax.minorticks_on()
            ax.set_xlabel(r'$h_x$($J_1$)')
            ax.set_ylabel(r'$\delta_{ТМС}, \%$')

            file_header = "t\t\tMR\t\terr\t\t"
            file_data = np.zeros(shape=(1, len(MR_Ps)))
            file_data[0] = x
            plot(ax, x=x, y=MR_Ps, yerr=MR_Ps_err, fmt=markers[0]+'-', label=r'$\delta_{P}$')

            file_data = np.concatenate(
                (file_data, np.array([MR_Ps]), np.array(object=[MR_Ps_err])), 
                    axis=0)

            ax.legend()
            x_min, x_max = ax.xaxis.get_data_interval()
            y_min, y_max = ax.yaxis.get_data_interval()
            x_tick_size = abs(x_max - x_min) / 10
            y_tick_size = abs(y_max - y_min) / 10

            plt.text(x=x_max - 3*x_tick_size, y=y_min + 4*y_tick_size, s=config.text() + "\n" + r"$A_{fb}=-0.8$")
            plt.tight_layout()
            plt.savefig(f"{config.path()}/MR_Ps_h.pdf", dpi=600)
            plt.close()

            np.savetxt(fname=f"{config.path()}/MR_Ps_h.dat", comments="",
                header=file_header,
                newline="\n", delimiter="\t", fmt=("%.2f\t" + "%.2f\t" * (file_data.shape[0] - 1)),
                X=np.rot90(file_data))
        except Exception as e:
            print(f"{type(e)} {e}")
