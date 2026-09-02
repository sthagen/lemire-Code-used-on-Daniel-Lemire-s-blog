#!/usr/bin/env python3
"""Bar charts of encoding/json vs encoding/json/v2 throughput."""
import collections
import os
import re
import statistics

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt

HERE = os.path.dirname(os.path.abspath(__file__))
RES = os.path.join(HERE, "results")

BENCH_RE = re.compile(r"^(Benchmark\S+)\s+\d+\s+\d+\s+ns/op\s+([0-9.]+)\s+MB/s")

plt.rcParams.update(
    {
        "font.size": 13,
        "axes.titlesize": 14,
        "axes.labelsize": 13,
        "xtick.labelsize": 12,
        "ytick.labelsize": 12,
        "legend.fontsize": 11,
    }
)


def parse(path):
    series = collections.defaultdict(list)
    with open(path) as f:
        for line in f:
            m = BENCH_RE.match(line)
            if m:
                name = re.sub(r"-\d+$", "", m.group(1))
                series[name].append(float(m.group(2)))
    return {k: statistics.median(v) for k, v in series.items()}


def load_host(host):
    return parse(os.path.join(RES, f"{host}-legacy.txt")), parse(
        os.path.join(RES, f"{host}-v1v2.txt")
    )


def value(legacy, v1v2, kind, impl, doc=None):
    if kind in ("UnmarshalRecords", "MarshalRecords"):
        if impl == "legacy":
            return legacy.get(f"Benchmark{kind}/{impl}", legacy[f"Benchmark{kind}"])
        return v1v2[f"Benchmark{kind}/{impl}"]
    src = legacy if impl == "legacy" else v1v2
    return src[f"Benchmark{kind}/{impl}/{doc}"]


DOCS = ["twitter", "canada", "citm_catalog"]
DOC_LABELS = ["twitter.json", "canada.json", "citm_catalog.json"]
IMPLS = ["legacy", "json", "jsonv2"]
IMPL_LABELS = ["json (legacy)", "json (Go 1.27)", "json/v2"]
COLORS = ["#8d9db6", "#4c6a92", "#2e7d32"]


def grouped(ax, legacy, v1v2, kind, ymax):
    x = list(range(len(DOCS)))
    width = 0.24
    for i, impl in enumerate(IMPLS):
        xs = [xi + (i - 1) * width for xi in x]
        ys = [value(legacy, v1v2, kind, impl, doc) for doc in DOCS]
        bars = ax.bar(xs, ys, width, color=COLORS[i], label=IMPL_LABELS[i])
        for bar, y in zip(bars, ys):
            ax.text(
                bar.get_x() + bar.get_width() / 2,
                y,
                f"{y:.0f}",
                ha="center",
                va="bottom",
                fontsize=9,
            )
    ax.set_xticks(x)
    ax.set_xticklabels(DOC_LABELS)
    ax.set_ylabel("MB/s")
    for s in ("top", "right"):
        ax.spines[s].set_visible(False)
    ax.yaxis.grid(True, linestyle=":", alpha=0.55)
    ax.set_axisbelow(True)
    ax.set_ylim(0, ymax)


def all_values(mac_l, mac_v, big_l, big_v):
    out = []
    for legacy, v1v2 in ((mac_l, mac_v), (big_l, big_v)):
        for kind in ("UnmarshalAny", "MarshalAny"):
            for impl in IMPLS:
                for doc in DOCS:
                    out.append(value(legacy, v1v2, kind, impl, doc))
        for kind in ("UnmarshalRecords", "MarshalRecords"):
            for impl in IMPLS:
                out.append(value(legacy, v1v2, kind, impl))
    return out


def main():
    mac_l, mac_v = load_host("mac")
    big_l, big_v = load_host("big4")
    ymax = max(all_values(mac_l, mac_v, big_l, big_v)) * 1.12

    fig, axes = plt.subplots(2, 2, figsize=(11.4, 8.8), sharey=True)
    grouped(axes[0, 0], mac_l, mac_v, "UnmarshalAny", ymax)
    axes[0, 0].set_title("Unmarshal into any  —  Apple M4 Max")
    grouped(axes[0, 1], big_l, big_v, "UnmarshalAny", ymax)
    axes[0, 1].set_title("Unmarshal into any  —  Xeon Gold 6548N")
    grouped(axes[1, 0], mac_l, mac_v, "MarshalAny", ymax)
    axes[1, 0].set_title("Marshal from any  —  Apple M4 Max")
    grouped(axes[1, 1], big_l, big_v, "MarshalAny", ymax)
    axes[1, 1].set_title("Marshal from any  —  Xeon Gold 6548N")
    for ax in axes[:, 1]:
        ax.set_ylabel("")
    handles, labels = axes[0, 0].get_legend_handles_labels()
    fig.legend(
        handles,
        labels,
        frameon=False,
        ncol=3,
        loc="upper center",
        bbox_to_anchor=(0.5, 0.97),
    )
    fig.suptitle(
        "Go 1.27: encoding/json vs encoding/json/v2", fontsize=16, y=0.995
    )
    fig.tight_layout(rect=(0, 0, 1, 0.93))
    out = os.path.join(HERE, "json-v1-v2.png")
    fig.savefig(out, dpi=170)
    print("wrote", out)

    fig, axes = plt.subplots(1, 2, figsize=(11.0, 4.5), sharey=True)
    kinds = ["UnmarshalRecords", "MarshalRecords"]
    labels = ["unmarshal", "marshal"]
    for ax, legacy, v1v2, title in (
        (axes[0], mac_l, mac_v, "Apple M4 Max"),
        (axes[1], big_l, big_v, "Xeon Gold 6548N"),
    ):
        x = list(range(2))
        width = 0.24
        for i, impl in enumerate(IMPLS):
            xs = [xi + (i - 1) * width for xi in x]
            ys = [value(legacy, v1v2, kind, impl) for kind in kinds]
            bars = ax.bar(xs, ys, width, color=COLORS[i], label=IMPL_LABELS[i])
            for bar, y in zip(bars, ys):
                ax.text(
                    bar.get_x() + bar.get_width() / 2,
                    y,
                    f"{y:.0f}",
                    ha="center",
                    va="bottom",
                    fontsize=10,
                )
        ax.set_xticks(x)
        ax.set_xticklabels(labels)
        ax.set_ylabel("MB/s")
        ax.set_title(f"slice of structs  —  {title}")
        for s in ("top", "right"):
            ax.spines[s].set_visible(False)
        ax.yaxis.grid(True, linestyle=":", alpha=0.55)
        ax.set_axisbelow(True)
        ax.set_ylim(0, ymax)
    axes[1].set_ylabel("")
    axes[0].legend(frameon=False, loc="upper left")
    fig.suptitle("Go 1.27: slice of typed records", fontsize=16, y=0.98)
    fig.tight_layout(rect=(0, 0, 1, 0.94))
    out = os.path.join(HERE, "json-records.png")
    fig.savefig(out, dpi=170)
    print("wrote", out)


if __name__ == "__main__":
    main()
