#pragma once

#include <QPointF>
#include <vector>

struct GAPathParams {
  int population_size = 80;
  double crossover_rate = 1;
  double mutation_rate = 0.2;
  int max_generations = 2000;
  QPointF depot{0.0, 0.0};
  int seed = 8964;  // -1 = non-deterministic, >= 0 = fixed seed for reproducibility
};

struct GAPathResult {
  std::vector<int> order;
  std::vector<bool> reversed;
  double total_deadhead = 0.0;
};

GAPathResult solvePolygonOrderGreedy(
    const std::vector<std::pair<QPointF, QPointF>>& endpoints,
    const QPointF& depot = QPointF{0.0, 0.0});

GAPathResult solvePolygonOrderGA(
    const std::vector<std::pair<QPointF, QPointF>>& endpoints,
    const GAPathParams& params = GAPathParams{});
