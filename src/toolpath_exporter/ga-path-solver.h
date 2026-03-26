#pragma once

#include <QPointF>
#include <vector>

struct GAPathParams {
  int population_size = 100;
  double crossover_rate = 0.85;
  double mutation_rate = 0.15;
  int max_generations = 2000;
  QPointF depot{0.0, 0.0};
};

struct GAPathResult {
  std::vector<int> order;
  std::vector<bool> reversed;
  double total_deadhead = 0.0;
};

GAPathResult solvePolygonOrderGA(
    const std::vector<std::pair<QPointF, QPointF>>& endpoints,
    const GAPathParams& params = GAPathParams{});
