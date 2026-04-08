#!/usr/bin/env python3
# -*- coding: utf-8 -*-



from __future__ import print_function, division
import numpy as np
import os

# =========================================================
# 1. 参数设置
# =========================================================
N = 150                      # 子载波数 / IFFT 点数
NUM_TRIALS = 5000           # 随机搜索次数，可改大一些，比如 20000
SEED = 42                   # 随机种子，方便复现
SAVE_DIR = "papr_search_results"

# 是否使用你当前的窗函数
USE_WINDOW = True
if USE_WINDOW:
    win = np.concatenate(([0.5], np.ones(N - 2), [0.5]))
else:
    win = np.ones(N)

# 固定置零的位置（与你当前代码一致）
ZERO_IDXS = [0, N // 2, N - 1]

# =========================================================
# 2. 工具函数
# =========================================================
def calc_papr_db(x):
    """
    计算复基带时域波形 x 的 PAPR (dB)
    """
    power = np.abs(x) ** 2
    papr = np.max(power) / np.mean(power)
    papr_db = 10.0 * np.log10(papr)
    return papr_db


def build_time_waveform_from_freq(Xf, win):
    """
    根据频域序列 Xf 生成时域波形：
    ifftshift -> ifft -> 乘窗
    """
    Xf_shifted = np.fft.ifftshift(Xf)
    xt = np.fft.ifft(Xf_shifted) * win
    return xt


def generate_random_bpsk_freq(N, zero_idxs, rng):
    """
    模式 A：
    随机生成频域 BPSK 序列，不强制 +1/-1 数量相同
    """
    Xf = np.sign(rng.rand(N) * 2 - 1)
    Xf[Xf == 0] = 1  # 极小概率防止 sign(0)=0
    for idx in zero_idxs:
        Xf[idx] = 0
    return Xf.astype(np.float64)


def generate_balanced_bpsk_freq(N, zero_idxs, rng):
    """
    模式 B：
    强制非零子载波上的 +1/-1 数量尽量相同
    """
    Xf = np.zeros(N, dtype=np.float64)

    valid_idxs = [i for i in range(N) if i not in zero_idxs]
    M = len(valid_idxs)

    num_pos = M // 2
    num_neg = M - num_pos

    symbols = np.array([1.0] * num_pos + [-1.0] * num_neg, dtype=np.float64)
    rng.shuffle(symbols)

    for k, idx in enumerate(valid_idxs):
        Xf[idx] = symbols[k]

    return Xf


def analyze_freq_sequence(Xf, zero_idxs):
    """
    简单统计频域上 +1/-1 个数
    """
    valid_idxs = [i for i in range(len(Xf)) if i not in zero_idxs]
    vals = Xf[valid_idxs]
    num_pos = np.sum(vals > 0)
    num_neg = np.sum(vals < 0)
    num_zero = np.sum(vals == 0)
    return int(num_pos), int(num_neg), int(num_zero)


def search_best_sequence(N, zero_idxs, win, num_trials, rng, mode_name):
    """
    搜索某种模式下 PAPR 最低的频域序列
    mode_name:
        "random"   -> 随机 ±1
        "balanced" -> 强制 +1/-1 数量平衡
    """
    best = {
        "papr_db": 1e9,
        "Xf": None,
        "xt": None,
        "trial": -1
    }

    papr_list = []

    for t in range(num_trials):
        if mode_name == "random":
            Xf = generate_random_bpsk_freq(N, zero_idxs, rng)
        elif mode_name == "balanced":
            Xf = generate_balanced_bpsk_freq(N, zero_idxs, rng)
        else:
            raise ValueError("Unknown mode_name: {}".format(mode_name))

        xt = build_time_waveform_from_freq(Xf, win)
        papr_db = calc_papr_db(xt)
        papr_list.append(papr_db)

        if papr_db < best["papr_db"]:
            best["papr_db"] = papr_db
            best["Xf"] = Xf.copy()
            best["xt"] = xt.copy()
            best["trial"] = t

    papr_arr = np.array(papr_list)

    stats = {
        "min_papr_db": float(np.min(papr_arr)),
        "max_papr_db": float(np.max(papr_arr)),
        "mean_papr_db": float(np.mean(papr_arr)),
        "std_papr_db": float(np.std(papr_arr)),
        "best_trial": int(best["trial"])
    }

    return best, stats, papr_arr


def save_complex_txt(filename, x):
    """
    保存复数序列为 txt: 每行 real imag
    """
    with open(filename, "w") as f:
        for val in x:
            f.write("{:.8f} {:.8f}\n".format(val.real, val.imag))


def save_real_txt(filename, x):
    """
    保存实数序列为 txt: 每行一个值
    """
    with open(filename, "w") as f:
        for val in x:
            f.write("{:.8f}\n".format(float(val)))


# =========================================================
# 3. 主程序
# =========================================================
def main():
    if not os.path.exists(SAVE_DIR):
        os.makedirs(SAVE_DIR)

    rng_random = np.random.RandomState(SEED)
    rng_balanced = np.random.RandomState(SEED)

    print("=" * 72)
    print("N                : {}".format(N))
    print("NUM_TRIALS       : {}".format(NUM_TRIALS))
    print("USE_WINDOW       : {}".format(USE_WINDOW))
    print("ZERO_IDXS        : {}".format(ZERO_IDXS))
    print("=" * 72)

    # -------------------------
    # 模式 A：不约束 +1/-1 数量
    # -------------------------
    best_random, stats_random, papr_random = search_best_sequence(
        N=N,
        zero_idxs=ZERO_IDXS,
        win=win,
        num_trials=NUM_TRIALS,
        rng=rng_random,
        mode_name="random"
    )

    # -------------------------
    # 模式 B：强制 +1/-1 平衡
    # -------------------------
    best_balanced, stats_balanced, papr_balanced = search_best_sequence(
        N=N,
        zero_idxs=ZERO_IDXS,
        win=win,
        num_trials=NUM_TRIALS,
        rng=rng_balanced,
        mode_name="balanced"
    )

    # =====================================================
    # 4. 结果打印
    # =====================================================
    print("\n[模式 A] 随机 ±1，不要求 +1/-1 数量相同")
    print("最小 PAPR (dB)   : {:.4f}".format(stats_random["min_papr_db"]))
    print("平均 PAPR (dB)   : {:.4f}".format(stats_random["mean_papr_db"]))
    print("最大 PAPR (dB)   : {:.4f}".format(stats_random["max_papr_db"]))
    print("PAPR 标准差 (dB) : {:.4f}".format(stats_random["std_papr_db"]))
    print("最佳序列 trial   : {}".format(stats_random["best_trial"]))

    pos_r, neg_r, zero_r = analyze_freq_sequence(best_random["Xf"], ZERO_IDXS)
    print("最佳序列中 +1 数 : {}".format(pos_r))
    print("最佳序列中 -1 数 : {}".format(neg_r))
    print("最佳序列中  0 数 : {}".format(zero_r))

    print("\n[模式 B] 强制 +1/-1 数量尽量相同")
    print("最小 PAPR (dB)   : {:.4f}".format(stats_balanced["min_papr_db"]))
    print("平均 PAPR (dB)   : {:.4f}".format(stats_balanced["mean_papr_db"]))
    print("最大 PAPR (dB)   : {:.4f}".format(stats_balanced["max_papr_db"]))
    print("PAPR 标准差 (dB) : {:.4f}".format(stats_balanced["std_papr_db"]))
    print("最佳序列 trial   : {}".format(stats_balanced["best_trial"]))

    pos_b, neg_b, zero_b = analyze_freq_sequence(best_balanced["Xf"], ZERO_IDXS)
    print("最佳序列中 +1 数 : {}".format(pos_b))
    print("最佳序列中 -1 数 : {}".format(neg_b))
    print("最佳序列中  0 数 : {}".format(zero_b))

    print("\n" + "=" * 72)
    if stats_balanced["min_papr_db"] < stats_random["min_papr_db"]:
        print("结论：在这次随机搜索中，'平衡 +1/-1' 的最优 PAPR 更低。")
    elif stats_balanced["min_papr_db"] > stats_random["min_papr_db"]:
        print("结论：在这次随机搜索中，'平衡 +1/-1' 不一定更优。")
    else:
        print("结论：两种模式最优 PAPR 恰好相同。")
    print("=" * 72)

    # =====================================================
    # 5. 保存结果
    # =====================================================
    # 保存最佳频域序列
    save_real_txt(os.path.join(SAVE_DIR, "best_random_freq.txt"), best_random["Xf"])
    save_real_txt(os.path.join(SAVE_DIR, "best_balanced_freq.txt"), best_balanced["Xf"])

    # 保存最佳时域波形
    save_complex_txt(os.path.join(SAVE_DIR, "best_random_time.txt"), best_random["xt"])
    save_complex_txt(os.path.join(SAVE_DIR, "best_balanced_time.txt"), best_balanced["xt"])

    # 保存所有 PAPR 结果，便于后续画CDF或直方图
    save_real_txt(os.path.join(SAVE_DIR, "papr_random_all.txt"), papr_random)
    save_real_txt(os.path.join(SAVE_DIR, "papr_balanced_all.txt"), papr_balanced)

    print("\n结果已保存到目录: {}".format(SAVE_DIR))
    print("包含：")
    print("  - best_random_freq.txt")
    print("  - best_balanced_freq.txt")
    print("  - best_random_time.txt")
    print("  - best_balanced_time.txt")
    print("  - papr_random_all.txt")
    print("  - papr_balanced_all.txt")


if __name__ == "__main__":
    main()