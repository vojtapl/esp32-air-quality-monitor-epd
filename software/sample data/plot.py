#!/usr/bin/env python
import csv
import datetime as dt

import matplotlib as mpl
import matplotlib.dates as mdates
import numpy as np

mpl.rcParams["figure.dpi"] = 300
import matplotlib.pyplot as plt

t = []
pm1 = []
pm2p5 = []
pm4 = []
pm10 = []
relative_humidity = []
temperature = []
voc_index = []
nox_index = []
co2 = []

DATE_FROM = dt.datetime.now() - dt.timedelta(hours=12)
DATE_TO = dt.datetime.now() - dt.timedelta(hours=0)


def calc_dew_point(t, rel_hum):
    b = 18.678
    c = 257.14
    d = 234.5

    gamma_m = np.log((rel_hum / 100) * np.exp((b - t / d) * (t / (c + t))))
    return (c * gamma_m) / (b - gamma_m)


def calc_humidex(t, rel_hum):
    t_dew = calc_dew_point(t, rel_hum)
    return t + 0.5555 * (
        6.11 * np.exp(5417.7530 * (1 / 273.15 - 1 / (273.15 + t_dew))) - 10
    )


# def reject_outliers(data, m=3):
#     return data[abs(data - np.mean(data)) < m*np.std(data)]

with open("data.csv", encoding="ascii") as input_file:
    reader = csv.reader(input_file, delimiter=",")
    for row in reader:
        read_date = dt.datetime.strptime(row[1], " %d/%m/%Y %H:%M:%S")
        # remove non-synced datetime (will start at unix 0)
        pm1_val = float(row[2])
        if (
            read_date < DATE_FROM or read_date > DATE_TO or abs(pm1_val - 6553.5) < 0.1
        ):  # max val
            continue

        t.append(read_date)
        pm1.append(pm1_val)
        pm2p5.append(float(row[3]))
        pm4.append(float(row[4]))
        pm10.append(float(row[5]))
        relative_humidity.append(float(row[6]))
        temperature.append(float(row[7]))
        voc_index.append(float(row[8]))
        nox_index.append(float(row[9]))
        co2.append(int(row[10]))

t = np.array(t)
pm1 = np.array(pm1)
pm2p5 = np.array(pm2p5)
pm4 = np.array(pm4)
pm10 = np.array(pm10)
relative_humidity = np.array(relative_humidity)
temperature = np.array(temperature)
voc_index = np.array(voc_index)
nox_index = np.array(nox_index)
co2 = np.array(co2)

# pm1 = reject_outliers(pm1)
# pm2p5 = reject_outliers(pm2p5)
# pm4 = reject_outliers(pm4)
# pm10 = reject_outliers(pm10)
# relative_humidity = reject_outliers(relative_humidity)
# temperature = reject_outliers(temperature)
# voc_index = reject_outliers(voc_index)
# nox_index = reject_outliers(nox_index)
# co2 = reject_outliers(co2)

humidex = calc_humidex(temperature, relative_humidity)

fig, ax = plt.subplots(nrows=3, ncols=3)
fig.autofmt_xdate()
fmt = mdates.DateFormatter("%m-%d %H:%M")

ax[0, 0].plot(t, pm1)
ax[0, 0].set_title("PM1 [μg/m³]")
ax[0, 0].fill_between(
    t, min(pm1), max(pm1), where=pm1 <= 9, facecolor="green", alpha=0.2
)
ax[0, 0].fill_between(
    t,
    min(pm1),
    max(pm1),
    where=np.logical_and(9 < pm1, pm1 <= 35.5),
    facecolor="yellow",
    alpha=0.2,
)
ax[0, 0].fill_between(
    t,
    min(pm1),
    max(pm1),
    where=np.logical_and(35.5 < pm1, pm1 <= 55.5),
    facecolor="orange",
    alpha=0.2,
)
ax[0, 0].fill_between(
    t,
    min(pm1),
    max(pm1),
    where=np.logical_and(55.5 < pm1, pm1 <= 125.5),
    facecolor="red",
    alpha=0.2,
)
ax[0, 0].fill_between(
    t,
    min(pm1),
    max(pm1),
    where=np.logical_and(125.5 < pm1, pm1 <= 225.5),
    facecolor="purple",
    alpha=0.2,
)
ax[0, 0].fill_between(
    t, min(pm1), max(pm1), where=225.5 < pm1, facecolor="maroon", alpha=0.2
)

ax[0, 1].plot(t, pm2p5)
ax[0, 1].set_title("PM2.5 [μg/m³]")
ax[0, 1].fill_between(
    t, min(pm2p5), max(pm2p5), where=pm2p5 <= 9, facecolor="green", alpha=0.2
)
ax[0, 1].fill_between(
    t,
    min(pm2p5),
    max(pm2p5),
    where=np.logical_and(9 < pm2p5, pm2p5 <= 35.5),
    facecolor="yellow",
    alpha=0.2,
)
ax[0, 1].fill_between(
    t,
    min(pm2p5),
    max(pm2p5),
    where=np.logical_and(35.5 < pm2p5, pm2p5 <= 55.5),
    facecolor="orange",
    alpha=0.2,
)
ax[0, 1].fill_between(
    t,
    min(pm2p5),
    max(pm2p5),
    where=np.logical_and(55.5 < pm2p5, pm2p5 <= 125.5),
    facecolor="red",
    alpha=0.2,
)
ax[0, 1].fill_between(
    t,
    min(pm2p5),
    max(pm2p5),
    where=np.logical_and(125.5 < pm2p5, pm2p5 <= 225.5),
    facecolor="purple",
    alpha=0.2,
)
ax[0, 1].fill_between(
    t, min(pm2p5), max(pm2p5), where=225.5 < pm2p5, facecolor="maroon", alpha=0.2
)

ax[0, 2].plot(t, pm4)
ax[0, 2].set_title("PM4 [μg/m³]")
ax[0, 2].fill_between(
    t, min(pm4), max(pm4), where=pm4 <= 9, facecolor="green", alpha=0.2
)
ax[0, 2].fill_between(
    t,
    min(pm4),
    max(pm4),
    where=np.logical_and(9 < pm4, pm4 <= 35.5),
    facecolor="yellow",
    alpha=0.2,
)
ax[0, 2].fill_between(
    t,
    min(pm4),
    max(pm4),
    where=np.logical_and(35.5 < pm4, pm4 <= 55.5),
    facecolor="orange",
    alpha=0.2,
)
ax[0, 2].fill_between(
    t,
    min(pm4),
    max(pm4),
    where=np.logical_and(55.5 < pm4, pm4 <= 125.5),
    facecolor="red",
    alpha=0.2,
)
ax[0, 2].fill_between(
    t,
    min(pm4),
    max(pm4),
    where=np.logical_and(125.5 < pm4, pm4 <= 225.5),
    facecolor="purple",
    alpha=0.2,
)
ax[0, 2].fill_between(
    t, min(pm4), max(pm4), where=225.5 < pm4, facecolor="maroon", alpha=0.2
)

ax[1, 0].plot(t, pm10)
ax[1, 0].set_title("PM10 [μg/m³]")
ax[1, 0].fill_between(
    t, min(pm10), max(pm10), where=pm10 <= 55, facecolor="green", alpha=0.2
)
ax[1, 0].fill_between(
    t,
    min(pm10),
    max(pm10),
    where=np.logical_and(55 < pm10, pm10 <= 155),
    facecolor="yellow",
    alpha=0.2,
)
ax[1, 0].fill_between(
    t,
    min(pm10),
    max(pm10),
    where=np.logical_and(155 < pm10, pm10 <= 255),
    facecolor="orange",
    alpha=0.2,
)
ax[1, 0].fill_between(
    t,
    min(pm10),
    max(pm10),
    where=np.logical_and(255 < pm10, pm10 <= 355),
    facecolor="red",
    alpha=0.2,
)
ax[1, 0].fill_between(
    t,
    min(pm10),
    max(pm10),
    where=np.logical_and(355 < pm10, pm10 <= 425),
    facecolor="purple",
    alpha=0.2,
)
ax[1, 0].fill_between(
    t, min(pm10), max(pm10), where=425 < pm10, facecolor="maroon", alpha=0.2
)

ax[1, 1].plot(t, relative_humidity)
ax[1, 1].set_title("Relative Humidity [%]")
ax[1, 1].fill_between(
    t,
    min(relative_humidity),
    max(relative_humidity),
    where=humidex <= 28,
    facecolor="green",
    alpha=0.2,
)
ax[1, 1].fill_between(
    t,
    min(relative_humidity),
    max(relative_humidity),
    where=np.logical_and(28 < humidex, humidex <= 32),
    facecolor="yellow",
    alpha=0.2,
)
ax[1, 1].fill_between(
    t,
    min(relative_humidity),
    max(relative_humidity),
    where=np.logical_and(32 < humidex, humidex <= 36),
    facecolor="orange",
    alpha=0.2,
)
ax[1, 1].fill_between(
    t,
    min(relative_humidity),
    max(relative_humidity),
    where=np.logical_and(36 < humidex, humidex <= 42),
    facecolor="red",
    alpha=0.2,
)
ax[1, 1].fill_between(
    t,
    min(relative_humidity),
    max(relative_humidity),
    where=np.logical_and(42 < humidex, humidex <= 45),
    facecolor="purple",
    alpha=0.2,
)
ax[1, 1].fill_between(
    t,
    min(relative_humidity),
    max(relative_humidity),
    where=45 < humidex,
    facecolor="maroon",
    alpha=0.2,
)

ax[1, 2].plot(t, temperature)
ax[1, 2].set_title("Temperature [°C]")
ax[1, 2].fill_between(
    t,
    min(temperature),
    max(temperature),
    where=humidex <= 28,
    facecolor="green",
    alpha=0.2,
)
ax[1, 2].fill_between(
    t,
    min(temperature),
    max(temperature),
    where=np.logical_and(28 < humidex, humidex <= 32),
    facecolor="yellow",
    alpha=0.2,
)
ax[1, 2].fill_between(
    t,
    min(temperature),
    max(temperature),
    where=np.logical_and(32 < humidex, humidex <= 36),
    facecolor="orange",
    alpha=0.2,
)
ax[1, 2].fill_between(
    t,
    min(temperature),
    max(temperature),
    where=np.logical_and(36 < humidex, humidex <= 42),
    facecolor="red",
    alpha=0.2,
)
ax[1, 2].fill_between(
    t,
    min(temperature),
    max(temperature),
    where=np.logical_and(42 < humidex, humidex <= 45),
    facecolor="purple",
    alpha=0.2,
)
ax[1, 2].fill_between(
    t,
    min(temperature),
    max(temperature),
    where=45 < humidex,
    facecolor="maroon",
    alpha=0.2,
)

ax[2, 0].plot(t, voc_index)
ax[2, 0].set_title("VOC index")
ax[2, 0].xaxis.set_major_formatter(fmt)
ax[2, 0].fill_between(
    t,
    min(voc_index),
    max(voc_index),
    where=voc_index <= 50,
    facecolor="green",
    alpha=0.2,
)
ax[2, 0].fill_between(
    t,
    min(voc_index),
    max(voc_index),
    where=np.logical_and(50 < voc_index, voc_index <= 100),
    facecolor="yellow",
    alpha=0.2,
)
ax[2, 0].fill_between(
    t,
    min(voc_index),
    max(voc_index),
    where=np.logical_and(100 < voc_index, voc_index <= 200),
    facecolor="orange",
    alpha=0.2,
)
ax[2, 0].fill_between(
    t,
    min(voc_index),
    max(voc_index),
    where=np.logical_and(200 < voc_index, voc_index <= 300),
    facecolor="red",
    alpha=0.2,
)
ax[2, 0].fill_between(
    t,
    min(voc_index),
    max(voc_index),
    where=np.logical_and(300 < voc_index, voc_index <= 400),
    facecolor="purple",
    alpha=0.2,
)
ax[2, 0].fill_between(
    t,
    min(voc_index),
    max(voc_index),
    where=400 < voc_index,
    facecolor="maroon",
    alpha=0.2,
)

ax[2, 1].plot(t, nox_index)
ax[2, 1].set_title("NOX index")
ax[2, 1].xaxis.set_major_formatter(fmt)
ax[2, 1].fill_between(
    t,
    min(nox_index),
    max(nox_index),
    where=nox_index <= 50,
    facecolor="green",
    alpha=0.2,
)
ax[2, 1].fill_between(
    t,
    min(nox_index),
    max(nox_index),
    where=np.logical_and(50 < nox_index, nox_index <= 100),
    facecolor="yellow",
    alpha=0.2,
)
ax[2, 1].fill_between(
    t,
    min(nox_index),
    max(nox_index),
    where=np.logical_and(100 < nox_index, nox_index <= 200),
    facecolor="orange",
    alpha=0.2,
)
ax[2, 1].fill_between(
    t,
    min(nox_index),
    max(nox_index),
    where=np.logical_and(200 < nox_index, nox_index <= 300),
    facecolor="red",
    alpha=0.2,
)
ax[2, 1].fill_between(
    t,
    min(nox_index),
    max(nox_index),
    where=np.logical_and(300 < nox_index, nox_index <= 400),
    facecolor="purple",
    alpha=0.2,
)
ax[2, 1].fill_between(
    t,
    min(nox_index),
    max(nox_index),
    where=400 < nox_index,
    facecolor="maroon",
    alpha=0.2,
)

ax[2, 2].plot(t, co2)
ax[2, 2].set_title("CO2 [ppm]")
ax[2, 2].xaxis.set_major_formatter(fmt)
ax[2, 2].fill_between(
    t, min(co2), max(co2), where=co2 <= 800, facecolor="green", alpha=0.2
)
ax[2, 2].fill_between(
    t,
    min(co2),
    max(co2),
    where=np.logical_and(800 < co2, co2 <= 1000),
    facecolor="yellow",
    alpha=0.2,
)
ax[2, 2].fill_between(
    t,
    min(co2),
    max(co2),
    where=np.logical_and(1000 < co2, co2 <= 1500),
    facecolor="orange",
    alpha=0.2,
)
ax[2, 2].fill_between(
    t,
    min(co2),
    max(co2),
    where=np.logical_and(1500 < co2, co2 <= 2000),
    facecolor="red",
    alpha=0.2,
)
ax[2, 2].fill_between(
    t,
    min(co2),
    max(co2),
    where=np.logical_and(2000 < co2, co2 <= 5000),
    facecolor="purple",
    alpha=0.2,
)
ax[2, 2].fill_between(
    t, min(co2), max(co2), where=5000 < co2, facecolor="maroon", alpha=0.2
)

plt.show()
