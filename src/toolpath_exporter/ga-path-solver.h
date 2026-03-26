#pragma once

#include <QPointF>
#include <vector>

struct GAPathParams {
  int population_size = 60;
  double crossover_rate = 0.85;
  double mutation_rate = 0.02;
  int max_generations = 500;
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
