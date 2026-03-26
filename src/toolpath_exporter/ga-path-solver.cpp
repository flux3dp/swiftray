#include "ga-path-solver.h"
#include <QDebug>
#include <QElapsedTimer>
#include <algorithm>
#include <cmath>
#include <numeric>
#include <random>

namespace {

inline double euclidean(const QPointF& a, const QPointF& b) {
  double dx = a.x() - b.x();
  double dy = a.y() - b.y();
  return std::sqrt(dx * dx + dy * dy);
}

struct EvalResult {
  double deadhead;
  std::vector<bool> dirs;  // true = reversed
};

// DP evaluation: find optimal direction for each polygon given a permutation order.
// dir 0 = forward (first→last), dir 1 = reversed (last→first).
// entry[d] = endpoint we enter from, exit[d] = endpoint we exit at.
EvalResult evaluate(const std::vector<int>& perm,
                    const std::vector<std::pair<QPointF, QPointF>>& ep,
                    const QPointF& depot) {
  const int k = static_cast<int>(perm.size());
  if (k == 0) return {0.0, {}};

  constexpr double INF = 1e15;
  double dp[2];
  std::vector<std::array<int, 2>> from(k, {-1, -1});

  // First polygon: cost to reach each direction from depot
  const auto& e0 = ep[perm[0]];
  dp[0] = euclidean(depot, e0.first);   // enter at first, exit at last
  dp[1] = euclidean(depot, e0.second);  // enter at last, exit at first

  for (int i = 1; i < k; i++) {
    const auto& prev = ep[perm[i - 1]];
    const auto& curr = ep[perm[i]];
    // exit points of prev: dir0 exits at .second, dir1 exits at .first
    QPointF exitPt[2] = {prev.second, prev.first};
    // entry points of curr: dir0 enters at .first, dir1 enters at .second
    QPointF entryPt[2] = {curr.first, curr.second};

    double nd[2] = {INF, INF};
    for (int d = 0; d < 2; d++) {
      for (int pd = 0; pd < 2; pd++) {
        if (dp[pd] >= INF) continue;
        double c = dp[pd] + euclidean(exitPt[pd], entryPt[d]);
        if (c < nd[d]) {
          nd[d] = c;
          from[i][d] = pd;
        }
      }
    }
    dp[0] = nd[0];
    dp[1] = nd[1];
  }

  // No return-to-depot: just pick the best final direction
  int fd = (dp[0] <= dp[1]) ? 0 : 1;
  double deadhead = dp[fd];

  // Backtrace to recover direction per polygon
  std::vector<bool> dirs(k);
  dirs[k - 1] = (fd == 1);
  for (int i = k - 1; i > 0; i--) {
    int prevDir = from[i][dirs[i] ? 1 : 0];
    dirs[i - 1] = (prevDir == 1);
  }

  return {deadhead, dirs};
}

using Perm = std::vector<int>;

Perm randPerm(int n, std::mt19937& rng) {
  Perm a(n);
  std::iota(a.begin(), a.end(), 0);
  for (int i = n - 1; i > 0; i--) {
    std::uniform_int_distribution<int> dist(0, i);
    int j = dist(rng);
    std::swap(a[i], a[j]);
  }
  return a;
}

// Order Crossover (OX)
Perm orderCrossover(const Perm& p1, const Perm& p2, std::mt19937& rng) {
  const int n = static_cast<int>(p1.size());
  std::uniform_int_distribution<int> dist(0, n - 1);
  int a = dist(rng), b = dist(rng);
  if (a > b) std::swap(a, b);

  Perm child(n, -1);
  std::vector<bool> used(n, false);
  for (int i = a; i <= b; i++) {
    child[i] = p1[i];
    used[p1[i]] = true;
  }

  int ci = (b + 1) % n;
  int pi = (b + 1) % n;
  while (true) {
    bool allUsed = true;
    for (int i = 0; i < n; i++) {
      if (!used[i]) { allUsed = false; break; }
    }
    if (allUsed) break;

    if (!used[p2[pi]]) {
      child[ci] = p2[pi];
      used[p2[pi]] = true;
      ci = (ci + 1) % n;
    }
    pi = (pi + 1) % n;
  }
  return child;
}

Perm inversionMutation(const Perm& p, std::mt19937& rng) {
  Perm a = p;
  const int n = static_cast<int>(a.size());
  std::uniform_int_distribution<int> dist(0, n - 1);
  int i = dist(rng), j = dist(rng);
  if (i > j) std::swap(i, j);
  std::reverse(a.begin() + i, a.begin() + j + 1);
  return a;
}

Perm insertionMutation(const Perm& p, std::mt19937& rng) {
  Perm a = p;
  const int n = static_cast<int>(a.size());
  std::uniform_int_distribution<int> dist(0, n - 1);
  int i = dist(rng), j = dist(rng);
  int val = a[i];
  a.erase(a.begin() + i);
  a.insert(a.begin() + j, val);
  return a;
}

int tournamentSelect(const std::vector<double>& costs, std::mt19937& rng, int k = 3) {
  std::uniform_int_distribution<int> dist(0, static_cast<int>(costs.size()) - 1);
  int best = dist(rng);
  for (int i = 1; i < k; i++) {
    int c = dist(rng);
    if (costs[c] < costs[best]) best = c;
  }
  return best;
}

// Greedy nearest-neighbor solver (also used to seed one GA individual)
GAPathResult solveGreedy(const std::vector<std::pair<QPointF, QPointF>>& ep,
                         const QPointF& depot) {
  const int n = static_cast<int>(ep.size());
  GAPathResult result;
  result.order.reserve(n);
  result.reversed.resize(n, false);

  std::vector<bool> visited(n, false);
  QPointF current = depot;

  for (int step = 0; step < n; step++) {
    double minDist = 1e15;
    int minIdx = -1;
    bool minRev = false;

    for (int i = 0; i < n; i++) {
      if (visited[i]) continue;
      double d0 = euclidean(current, ep[i].first);
      if (d0 < minDist) { minDist = d0; minIdx = i; minRev = false; }
      double d1 = euclidean(current, ep[i].second);
      if (d1 < minDist) { minDist = d1; minIdx = i; minRev = true; }
    }

    visited[minIdx] = true;
    result.order.push_back(minIdx);
    result.reversed[minIdx] = minRev;
    current = minRev ? ep[minIdx].first : ep[minIdx].second;
    result.total_deadhead += minDist;
  }

  return result;
}

}  // namespace

GAPathResult solvePolygonOrderGA(
    const std::vector<std::pair<QPointF, QPointF>>& endpoints,
    const GAPathParams& params) {
  const int n = static_cast<int>(endpoints.size());
  if (n == 0) return {};

  QElapsedTimer timer;
  timer.start();

  // For small inputs, just use greedy
  if (n < 4) {
    auto result = solveGreedy(endpoints, params.depot);
    qDebug() << "[GA] n=" << n << "(<4, greedy fallback), deadhead=" << result.total_deadhead
             << ", time=" << timer.elapsed() << "ms";
    return result;
  }

  thread_local std::mt19937 rng{std::random_device{}()};

  const int POP = params.population_size;
  const double CX = params.crossover_rate;
  const double MUT = params.mutation_rate;
  const int MAXG = std::min(std::max(params.max_generations, n * 100), int(1e6));
  const QPointF& depot = params.depot;

  // Build greedy solution to seed one individual
  GAPathResult greedyResult = solveGreedy(endpoints, depot);
  qDebug() << "[GA] n=" << n << ", greedy seed deadhead=" << greedyResult.total_deadhead;
  // Convert greedy order to a permutation
  Perm greedySeed = Perm(greedyResult.order.begin(), greedyResult.order.end());

  // Initialize population
  std::vector<Perm> pop(POP);
  pop[0] = greedySeed;
  for (int i = 1; i < POP; i++) {
    pop[i] = randPerm(n, rng);
  }

  Perm bestPerm;
  double bestCost = 1e15;
  std::vector<bool> bestDirs;
  int stale = 0;
  int staleSinceTribulation = 0;
  double checkpointCost = 1e15;
  qint64 nextCheckpointMs = 500;

  std::uniform_real_distribution<double> realDist(0.0, 1.0);

  for (int gen = 0; gen < MAXG; gen++) {
    // Evaluate all individuals
    std::vector<double> costs(POP);
    std::vector<std::vector<bool>> allDirs(POP);
    for (int i = 0; i < POP; i++) {
      auto res = evaluate(pop[i], endpoints, depot);
      costs[i] = res.deadhead;
      allDirs[i] = std::move(res.dirs);
    }

    // Track best
    bool improved = false;
    for (int i = 0; i < POP; i++) {
      if (costs[i] < bestCost) {
        bestCost = costs[i];
        bestPerm = pop[i];
        bestDirs = allDirs[i];
        improved = true;
        stale = 0;
        staleSinceTribulation = 0;
      }
    }
    if (!improved) {
      stale++;
      staleSinceTribulation++;
    }

    if (gen % 50 == 0 || improved) {
      qDebug() << "[GA] gen=" << gen << ", best=" << bestCost
               << ", stale=" << stale << ", elapsed=" << timer.elapsed() << "ms";
    }

    // Early termination: stale generations
    if (stale > POP * 7) {
      qDebug() << "[GA] Early termination (stale) at gen=" << gen << ", stale=" << stale;
      break;
    }

    // Early termination: <0.5% improvement per 500ms interval
    qint64 elapsed = timer.elapsed();
    if (elapsed >= nextCheckpointMs) {
      double improvementPct = (checkpointCost > 0 && checkpointCost < 1e15)
          ? (1.0 - bestCost / checkpointCost) * 100.0
          : 100.0;
      qDebug() << "[GA] Checkpoint at" << elapsed << "ms: best=" << bestCost
               << ", prev=" << checkpointCost << ", improvement=" << improvementPct << "%";
      if (checkpointCost < 1e15 && improvementPct < 1) {
        qDebug() << "[GA] Early termination (convergence) at gen=" << gen
                 << ", improvement=" << improvementPct << "% < 1%";
        break;
      }
      checkpointCost = bestCost;
      nextCheckpointMs = elapsed + 1000;
    }

    // Find current generation's best for elitism
    int bi = 0;
    for (int i = 1; i < POP; i++) {
      if (costs[i] < costs[bi]) bi = i;
    }

    // Build next generation
    std::vector<Perm> np;
    np.reserve(POP);
    np.push_back(pop[bi]);                         // elite
    np.push_back(inversionMutation(pop[bi], rng)); // mutant of elite
    np.push_back(insertionMutation(pop[bi], rng)); // mutant of elite

    while (static_cast<int>(np.size()) < POP) {
      int p1i = tournamentSelect(costs, rng);
      int p2i = tournamentSelect(costs, rng);
      Perm ch = (realDist(rng) < CX) ? orderCrossover(pop[p1i], pop[p2i], rng) : pop[p1i];
      if (realDist(rng) < MUT) {
        ch = (realDist(rng) > 0.5) ? inversionMutation(ch, rng) : insertionMutation(ch, rng);
      }
      np.push_back(std::move(ch));
    }

    // Tribulation: refresh bottom half when stale since last tribulation
    if (staleSinceTribulation > static_cast<int>(POP * 1.5)) {
      for (int i = POP / 2; i < POP; i++) {
        np[i] = randPerm(n, rng);
      }
      staleSinceTribulation = 0;
    }

    pop = std::move(np);
  }

  qDebug() << "[GA] Done. n=" << n << ", best deadhead=" << bestCost
           << ", greedy deadhead=" << greedyResult.total_deadhead
           << ", improvement=" << (1.0 - bestCost / greedyResult.total_deadhead) * 100.0 << "%"
           << ", total time=" << timer.elapsed() << "ms";

  // Build result from best permutation
  GAPathResult result;
  result.order.resize(n);
  result.reversed.resize(n, false);
  result.total_deadhead = bestCost;

  for (int i = 0; i < n; i++) {
    result.order[i] = bestPerm[i];
    result.reversed[i] = bestDirs[i];
  }

  return result;
}
