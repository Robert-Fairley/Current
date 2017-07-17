/*******************************************************************************
The MIT License (MIT)

Copyright (c) 2016 Dmitry "Dima" Korolev <dmitry.korolev@gmail.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*******************************************************************************/

// Step 3: Train the classification model to caterogize Fisher's Irises data into their respective classes.
// NOTE(dkorolev): To look at model optimization code only, diff `step3_optimize.cc` against `step3_render.cc`.

#include "../../typesystem/struct.h"
#include "../../blocks/HTTP/api.h"
#include "../../bricks/graph/gnuplot.h"
#include "../../bricks/file/file.h"
#include "../../bricks/dflags/dflags.h"
#include "../../FnCAS/fncas.h"

#include "data/dataset.h"
using IrisFlower = Schema_Element_Object;  // Using the default type name from the autogenerated schema.

DEFINE_string(input, "data/dataset.json", "The path to the input data file.");
DEFINE_uint16(port, 3001, "The port to run the server on.");
DEFINE_bool(train_descriptive, true, "Unset this flag to train the discriminant model right away.");
DEFINE_bool(train_discriminant, true, "Unset this flag to train the descriptive model only.");
DEFINE_bool(dump_regions_to_stderr, false, "Set this flag to dump the 'picture' of class regions to standard error.");

struct Label {
  const std::string name;
  const std::string color;
  const std::string pt;  // gnuplot's "point type".
};
static std::vector<Label> labels = {
    {"setosa", "rgb '#ff0000'", "6"}, {"versicolor", "rgb '#00c000'", "8"}, {"virginica", "rgb '#0000c0'", "10"}};

struct Feature {
  double IrisFlower::*mem_ptr;
  const size_t dimension_index;
  const std::string name;
};

static std::vector<std::pair<std::string, Feature>> features_list = {{"SL", {&IrisFlower::SL, 0u, "Sepal.Length"}},
                                                                     {"SW", {&IrisFlower::SW, 1u, "Sepal.Width"}},
                                                                     {"PL", {&IrisFlower::PL, 2u, "Petal.Length"}},
                                                                     {"PW", {&IrisFlower::PW, 3u, "Petal.Width"}}};
static std::map<std::string, Feature> features(features_list.begin(), features_list.end());

static std::map<std::string, size_t> label_indexes{{"setosa", 0u}, {"versicolor", 1u}, {"virginica", 2u}};

struct NoModelVisualizer {
  template <typename T1, typename T2>
  NoModelVisualizer(T1&&, T2&&) {}
  void AddPlots(current::gnuplot::GNUPlot& unused_plot, size_t unused_dimension_x, size_t unused_dimension_y) const {
    static_cast<void>(unused_plot);
    static_cast<void>(unused_dimension_x);
    static_cast<void>(unused_dimension_y);
  }
};

template <typename T = NoModelVisualizer>
Response Plot(const std::vector<IrisFlower>& flowers,
              const std::string& x,
              const std::string& y,
              bool nolegend,
              size_t image_size,
              double point_size,
              T&& model_visualizer = T(),
              const std::string& output_format = "svg",
              const std::string& content_type = current::net::constants::kDefaultSVGContentType) {
  try {
    const auto ix = features.at(x);
    const auto iy = features.at(y);
    const double IrisFlower::*px = ix.mem_ptr;
    const double IrisFlower::*py = iy.mem_ptr;
    using namespace current::gnuplot;
    GNUPlot plot;
    if (nolegend) {
      plot.NoBorder().NoTics().NoKey();
    } else {
      plot.Title("Iris Data").Grid("back").XLabel(features.at(x).name).YLabel(features.at(y).name);
    }
    plot.ImageSize(image_size).OutputFormat(output_format);
    for (const auto& label : labels) {
      auto plot_data = WithMeta([&label, &flowers, px, py](Plotter& p) {
        for (const auto& flower : flowers) {
          if (flower.Label == label.name) {
            p(flower.*px, flower.*py);
          }
        }
      });
      plot.Plot(plot_data.AsPoints()
                    .Name(label.name)
                    .Color(label.color)
                    .Extra("pt " + label.pt + (point_size ? "ps " + current::ToString(point_size) : "")));
    }
    model_visualizer.AddPlots(plot, ix.dimension_index, iy.dimension_index);
    return Response(static_cast<std::string>(plot), HTTPResponseCode.OK, content_type);
  } catch (const std::out_of_range&) {
    return Response("Invalid dimension.", HTTPResponseCode.BadRequest);
  }
}

enum class ComputationType { TrainDescriptiveModel, TrainDiscriminantModel, ComputeAccuracy };

// The four-dimensional Gaussian with all radii of one has the normalization coefficient of `pi^2`:
//
// * the definite integral of `exp(-(x*x + y*y + z*z + t*t)) / (pi^2)` is `1`.
// * Ref.: http://www.wolframalpha.com/input/?i=exp(-(x*x%2By*y%2Bz*z%2Bt*t))%2F(pi^2)
//
// The four-dimensional Gaussian with the radii being `r` has the normalization coefficient of `pi^2 * r^4`.
// * Same integral is `1` for `exp(-((x/2)*(x/2) + (y/2)*(y/2) + (z/2)*(z/2) + (t/2)*(t/2))) / (pi^2 * 16)`.
// * Ref.: http://www.wolframalpha.com/input/?i=exp(-((x/2)*(x/2)%2B(y/2)*(y/2)%2B(z/2)*(z/2)%2B(t/2)*(t/2)))/(pi^2*16)
//
// For all the radii being `r`, I move the coefficient under `exp(-(...)^2)`:
// * Same integral is `1` for `exp(-(x*x+y*y+z*z+t*t) / (r^2)) / (pi^2*(r^4))`.
// * Ref.: http://www.wolframalpha.com/input/?i=exp(-(x*x%2By*y%2Bz*z%2Bt*t)+%2F+(2^2))+%2F+(pi^2*(2^4))
//         (Replace `2` by, say, `5`, i.e. { `2^2`, `2^4` -> `5^2`, `5^4` }, and the integral will stay `1`.
//
// Thus, as an implementation detail, model parameters to optimize, the per-coordinate radii, are stored
// as `-log(sqr(r_i))`. This way, where `x` is the corresponding per-cluster radius-parameter:
// * The negated exponent argument, which is `1/(r^2)`, is calculated as `exp(x)`, and stored as `inv_r_squared`.
// * The normalization parameter, `k`, which `pi^2` in the denominator is multiplied by, is `exp(x*2)`.
// * The radius itself (which is only used for visualization purposes) is `exp(-x/2)`.
//
// The presence of the seemingly useless exponent in the formula also ensures the optimization
// can not accidentlaly enter the "negative radius" domain.
template <typename T>
struct WeightsComputer {
  const std::vector<T>& x;
  T inv_r_squared[3];
  T k[3];

  // For a 4D Gaussian, `exp(-(distance_squared / radius_squared))`, the normalization coefficient
  // is `radius_squared ^ (-2)`. As an implementation detail, use inverted values.
  explicit WeightsComputer(const std::vector<T>& x) : x(x) {
    for (size_t c = 0; c < 3; ++c) {
      inv_r_squared[c] = fncas::exp(x[c * 5 + 4]);
      k[c] = fncas::exp(x[c * 5 + 4] * 2);
    }
  }

  T WeightInClass(const IrisFlower& flower, size_t c) {
    const T dsl_squared = fncas::sqr(flower.SL - x[c * 5 + 0]);
    const T dsw_squared = fncas::sqr(flower.SW - x[c * 5 + 1]);
    const T dpl_squared = fncas::sqr(flower.PL - x[c * 5 + 2]);
    const T dpw_squared = fncas::sqr(flower.PW - x[c * 5 + 3]);
    const T distance_squared = dsl_squared + dsw_squared + dpl_squared + dpw_squared;
    return fncas::exp(-distance_squared * inv_r_squared[c]) * k[c] * (1.0 / (M_PI * M_PI));
  }
};

Response PlotAccuracy(const std::vector<IrisFlower>& flowers,
                      const std::vector<double>& model,
                      size_t predicted_label,
                      size_t actual_label,
                      size_t bins,
                      bool nolegend,
                      size_t image_size,
                      double line_width,
                      const std::string& output_format = "svg",
                      const std::string& content_type = current::net::constants::kDefaultSVGContentType) {
  if (predicted_label < 3 && actual_label < 3 && (bins >= 2 && bins <= 100)) {
    using namespace current::gnuplot;
    GNUPlot plot;
    if (nolegend) {
      plot.NoBorder().NoTics().NoKey();
    } else {
      plot.Title("True label '" + labels[actual_label].name + "', label weight by model prediction for '" +
                 labels[predicted_label].name + "'.")
          .Grid("back")
          .XLabel("Probability")
          .YLabel("Count");
    }
    plot.ImageSize(image_size).OutputFormat(output_format);

    auto plot_data = WithMeta([&](Plotter& p) {
      WeightsComputer<double> computer(model);
      const size_t N = bins;
      std::vector<size_t> count(N, 0u);
      std::vector<double> wc(3);
      double ws;
      for (const auto& flower : flowers) {
        if (flower.Label == labels[actual_label].name) {
          ws = 0.0;
          for (size_t c = 0; c < 3; ++c) {
            const double w = computer.WeightInClass(flower, c);
            wc[c] = w;
            ws += w;
          }
          const double p = wc[predicted_label] / ws;
          size_t bin = static_cast<size_t>(p * N);
          if (bin == N) {
            bin = N - 1;
          }
          assert(bin >= 0);
          assert(bin < N);
          ++count[bin];
        }
      }
      const double k = 1.0 / N;
      for (size_t i = 0; i < N; ++i) {
        p(i * k, 0);
        p(i * k, count[i]);
        p((i + 1) * k, count[i]);
      }
      p(1, 0);
      p(0, 0);
    });
    plot_data.Name(labels[actual_label].name).LineWidth(line_width).Color(labels[actual_label].color);
    plot.Plot(plot_data);
    return Response(static_cast<std::string>(plot), HTTPResponseCode.OK, content_type);
  } else {
    return Response("Invalid dimension.", HTTPResponseCode.BadRequest);
  }
}

CURRENT_STRUCT(MinMaxAvg) {
  CURRENT_FIELD(min, double);
  CURRENT_FIELD(max, double);
  CURRENT_FIELD(avg, double);
};

struct MinMaxAvgStats {
  const std::vector<MinMaxAvg> flowers_stats;
  static std::vector<MinMaxAvg> ComputeStats(const std::vector<IrisFlower>& flowers) {
    std::vector<MinMaxAvg> result(4);
    for (size_t d = 0; d < 4; ++d) {
      result[d].min = +1e100;
      result[d].max = -1e100;
      result[d].avg = 0.0;
    }
    for (const auto& flower : flowers) {
      for (size_t d = 0; d < 4; ++d) {
        const double IrisFlower::*p = features_list[d].second.mem_ptr;
        const double x = flower.*p;
        result[d].min = std::min(result[d].min, x);
        result[d].max = std::max(result[d].max, x);
        result[d].avg += x;
      }
    }
    for (size_t d = 0; d < 4; ++d) {
      result[d].avg /= flowers.size();
    }
    return result;
  }

  MinMaxAvgStats(const std::vector<IrisFlower>& flowers) : flowers_stats(ComputeStats(flowers)) {}

  const MinMaxAvg& operator[](size_t i) const { return flowers_stats[i]; }
};

struct FunctionToOptimize {
  const std::vector<IrisFlower>& flowers;
  const ComputationType computation_type;

  FunctionToOptimize(const std::vector<IrisFlower>& flowers, ComputationType computation_type)
      : flowers(flowers), computation_type(computation_type) {}

#if 0
  // A toy example: Optimize a simple three-parameter function with a clear maximum at {1,2,3}.
  static std::vector<double> StartingPoint(const std::vector<IrisFlower>&) { return {0, 0, 0}; }
  template <typename T>
  T ObjectiveFunction(const std::vector<T>& parameters) const {
    return -fncas::exp(fncas::sqr(parameters[0] - 1) + fncas::sqr(parameters[1] - 2) + fncas::sqr(parameters[2] - 3));
  }
  using ModelVisualizer = NoModelVisualizer;
#else
  // A real-life example: assume each of the three classes is defined by a hypersphere-shaped Gaussian.
  // The five parameters per each class are the centrer { SL, SW, PL, PW } and the raduis of the Gaussian.
  // The raduis parameter is exponentiated prior to optimizations, to ensure the resulting radius is positive.
  // Two models are then trained, the second one from the point found by the first one:
  // Model 1, the descriptive one: Find the best Gaussian per class (could be computed analytically, but why bother?)
  // Model 2, the discriminant one: Move the best per-class Gaussians to solve the actual classification problem.
  static std::vector<double> StartingPoint(const std::vector<IrisFlower>& flowers) {
    // return std::vector<double>(3 * 5, 0.0);
    // Start with wide Gaussians.
    std::vector<double> point;
    std::vector<std::vector<double>> center(3);
    for (size_t c = 0; c < 3; ++c) {
      for (size_t d = 0; d < 4; ++d) {
        const double IrisFlower::*p = features_list[d].second.mem_ptr;
        double sum_x = 0.0;
        size_t n = 0.0;
        for (const auto& flower : flowers) {
          const auto label_cit = label_indexes.find(flower.Label);
          if (label_cit != label_indexes.end() && label_cit->second == c) {
            const double x = flower.*p;
            sum_x += x;
            ++n;
          }
        }
        assert(n > 0);
        const double mean_x = sum_x / n;
        center[c].push_back(mean_x);
        point.push_back(mean_x);
      }
      point.push_back(-2.0);  // `-log(r ^ 2)` == `-2`, initial `r` = `exp(1)`, a wide Gaussian to start from.
    }

    // TODO(dkorolev): The closed-form solution for the starting point in the descriptive case.

    return point;
  }

  template <typename T>
  fncas::optimize::ObjectiveFunctionValue<T> ObjectiveFunction(const std::vector<T>& x) const {
    T negative_penalty_descriptive = 0.0;
    T negative_penalty_discriminant = 0.0;
    WeightsComputer<T> computer(x);
    size_t valid_examples = 0;
    for (const auto& flower : flowers) {
      const auto label_cit = label_indexes.find(flower.Label);
      if (label_cit != label_indexes.end()) {
        ++valid_examples;
        T w[3];
        T ws = 0.0;
        for (size_t c = 0; c < 3; ++c) {
          w[c] = computer.WeightInClass(flower, c);
          ws += w[c];
        }
        negative_penalty_descriptive += fncas::log(w[label_cit->second]);
        negative_penalty_discriminant += fncas::log(w[label_cit->second] / ws);
      }
    }
    assert(valid_examples > 0u);

    const T accuracy = fncas::exp(negative_penalty_discriminant / valid_examples);

    return fncas::optimize::ObjectiveFunctionValue<T>([&]() {
      if (computation_type == ComputationType::ComputeAccuracy) {
        // Return the probability, and the starting point should be `(1/3)`.
        return accuracy;
      } else {
        // Just return the non-normalized number, for optimization purposes.
        return computation_type == ComputationType::TrainDescriptiveModel ? negative_penalty_descriptive
                                                                          : negative_penalty_discriminant;
      }
    }())
        .AddPoint("accuracy", accuracy)
        .AddPoint("penalty_descriptive", -negative_penalty_descriptive)
        .AddPoint("penalty_discriminant", -negative_penalty_discriminant);
  }

  // Render the Gaussians as ellipses.
  struct ModelVisualizer {
    const std::vector<double> parameters;
    const MinMaxAvgStats& flowers_stats;

    ModelVisualizer(const std::vector<double>& parameters, const MinMaxAvgStats& flowers_stats)
        : parameters(parameters), flowers_stats(flowers_stats) {}

    void AddPlots(current::gnuplot::GNUPlot& plot, size_t dimension_x, size_t dimension_y) const {
      using namespace current::gnuplot;

      for (size_t c = 0; c < 3; ++c) {
        const double x = parameters[c * 5 + dimension_x];
        const double y = parameters[c * 5 + dimension_y];
        const double r = std::exp(parameters[c * 5 + 4] / 2) * 0.25;  // Just the visualization coefficient. -- D.K.
        auto plot_data = WithMeta([x, y, r](Plotter& p) {
          size_t n = 100;
          for (size_t i = 0; i <= n; ++i) {
            const double phi = 1.0 * M_PI * 2 * i / n;
            p(x + r * cos(phi), y + r * sin(phi));
          }
        });
        plot.Plot(plot_data.Name("Model for " + labels[c].name).Color(labels[c].color).LineWidth(3.5));
      }

      if (FLAGS_dump_regions_to_stderr) {
        // NOTE(dkorolev): Using the average for the other two dimensions is not good.
        // Need something a tad more sophisticated. I'm on it.
        WeightsComputer<double> computer(parameters);
        IrisFlower flower;
        for (size_t c = 0; c < 4; ++c) {
          flower.*features_list[c].second.mem_ptr = flowers_stats[c].avg;
        }
        const size_t N = 32;
        for (size_t iy = 0; iy <= N; ++iy) {
          // Top-to bottom mathematically.
          // const double y = (flowers_stats[dimension_y].min * (N - iy) + flowers_stats[dimension_y].max * iy) / N;
          const double y = (flowers_stats[dimension_y].min * iy + flowers_stats[dimension_y].max * (N - iy)) / N;
          for (size_t ix = 0; ix <= N; ++ix) {
            const double x = (flowers_stats[dimension_x].min * (N - ix) + flowers_stats[dimension_x].max * ix) / N;
            flower.*features_list[dimension_x].second.mem_ptr = x;
            flower.*features_list[dimension_y].second.mem_ptr = y;
            const double a = computer.WeightInClass(flower, 0);
            const double b = computer.WeightInClass(flower, 1);
            const double c = computer.WeightInClass(flower, 2);
            const double m = std::max(a, std::max(b, c));
            std::cerr << (m == a ? 'A' : (m == b ? 'B' : 'C'));
          }
          std::cerr << std::endl;
        }
      }
    }
  };
#endif
};

int main(int argc, char** argv) {
  ParseDFlags(&argc, &argv);

  const auto flowers = ParseJSON<std::vector<IrisFlower>>(current::FileSystem::ReadFileAsString(FLAGS_input));
  const auto flowers_stats = MinMaxAvgStats(flowers);

  std::cout << "Read " << flowers.size() << " flowers." << std::endl;
  std::cout << "Stats: " << JSON(flowers_stats.flowers_stats) << std::endl;

  // Uncomment the next line to see the training log.
  // fncas::impl::ScopedLogToStderr log_fncas_to_stderr_scope;

  std::vector<double> descriptive_optimization_accuracy_plot;
  std::vector<double> discriminant_optimization_accuracy_plot;

  const std::vector<double> intermediate_point = [&]() {
    if (FLAGS_train_descriptive) {
      const auto starting_point = FunctionToOptimize::StartingPoint(flowers);
      std::cout << "The starting point is: " << JSON(starting_point) << std::endl;
      std::cout << "Prior to the optimization, the objective function is: "
                << FunctionToOptimize(flowers, ComputationType::ComputeAccuracy).ObjectiveFunction(starting_point).value
                << std::endl;
      const auto descriptive_optimization_result =
          fncas::optimize::DefaultOptimizer<FunctionToOptimize, fncas::OptimizationDirection::Maximize>(
              fncas::optimize::OptimizerParameters().TrackOptimizationProgress(),
              flowers,
              ComputationType::TrainDescriptiveModel).Optimize(starting_point);
      descriptive_optimization_accuracy_plot =
          Value(descriptive_optimization_result.progress).additional_values.at("accuracy");
      const std::vector<double> intermediate_point = descriptive_optimization_result.point;
      std::cout << "After training the descriptive model, the objective function is: "
                << FunctionToOptimize(flowers, ComputationType::ComputeAccuracy)
                       .ObjectiveFunction(intermediate_point)
                       .value << std::endl;
      std::cout << "The intermediate parameters vector is: " << JSON(intermediate_point) << std::endl;
      return intermediate_point;
    } else {
      return FunctionToOptimize::StartingPoint(flowers);
    }
  }();
  const std::vector<double> final_point = [&]() {
    if (FLAGS_train_discriminant) {
      const auto discriminant_optimization_result =
          fncas::optimize::DefaultOptimizer<FunctionToOptimize, fncas::OptimizationDirection::Maximize>(
              fncas::optimize::OptimizerParameters().TrackOptimizationProgress(),
              flowers,
              ComputationType::TrainDiscriminantModel).Optimize(intermediate_point);
      discriminant_optimization_accuracy_plot =
          Value(discriminant_optimization_result.progress).additional_values.at("accuracy");
      const std::vector<double> final_point = discriminant_optimization_result.point;
      std::cout << "After training the descriminant model, the objective function is: "
                << FunctionToOptimize(flowers, ComputationType::ComputeAccuracy).ObjectiveFunction(final_point).value
                << std::endl;
      std::cout << "The optimal parameters vector is: " << JSON(final_point) << std::endl;
      return final_point;
    } else {
      // Skip the second stage of training entirely.
      return intermediate_point;
    }
  }();

  auto& http = HTTP(FLAGS_port);

  const auto scope =
      http.Register("/",
                    [&flowers, &flowers_stats, final_point](Request r) {
                      if (r.url.query.has("x") && r.url.query.has("y")) {
                        r(Plot(flowers,
                               r.url.query["x"],
                               r.url.query["y"],
                               r.url.query.has("nolegend"),
                               current::FromString<size_t>(r.url.query.get("dim", "800")),
                               current::FromString<double>(r.url.query.get("ps", "1.75")),
                               FunctionToOptimize::ModelVisualizer(final_point, flowers_stats)));
                      } else {
                        std::string html;
                        html += "<!doctype html>\n";
                        html += "<table border=1>\n";
                        for (size_t y = 0; y < 4; ++y) {
                          html += "  <tr>\n";
                          for (size_t x = 0; x < 4; ++x) {
                            if (x == y) {
                              const auto text = features_list[x].second.name;
                              html += "    <td align=center valign=center><h3><pre>" + text + "</pre></h1></td>\n";
                            } else {
                              const std::string img_a = "?x=" + features_list[x].first + "&y=" + features_list[y].first;
                              const std::string img_src = img_a + "&dim=250&nolegend&ps=1";
                              html += "    <td><a href='" + img_a + "'><img src='" + img_src + "' /></a></td>\n";
                            }
                          }
                          html += "  </tr>\n";
                        }
                        html += "</table>\n";
                        r(html, HTTPResponseCode.OK, current::net::constants::kDefaultHTMLContentType);
                      }
                    }) +
      http.Register("/accuracy",
                    [&flowers, &flowers_stats, final_point](Request r) {
                      if (r.url.query.has("predicted") && r.url.query.has("actual")) {
                        r(PlotAccuracy(flowers,
                                       final_point,
                                       current::FromString<size_t>(r.url.query["predicted"]),
                                       current::FromString<size_t>(r.url.query["actual"]),
                                       current::FromString<size_t>(r.url.query.get("bins", "50")),
                                       r.url.query.has("nolegend"),
                                       current::FromString<size_t>(r.url.query.get("dim", "800")),
                                       current::FromString<double>(r.url.query.get("lw", "3"))));
                      } else {
                        std::string html;
                        html += "<!doctype html>\n";
                        html += "<table border=1>\n";
                        html += "  <tr><td></td>";
                        for (size_t i = 0; i < 3; ++i) {
                          html += "<td align=center>Histogram of P(<b>" + labels[i].name + "</b>) values.</td>";
                        }
                        html += "</tr>\n";
                        for (size_t y = 0; y < 3; ++y) {
                          html += "  <tr><td><p style='transform:rotate(270deg);text-align:center;'><b>" +
                                  labels[y].name + "</b> flowers.</p></td>";
                          for (size_t x = 0; x < 3; ++x) {
                            const std::string img_a =
                                "accuracy?predicted=" + current::ToString(x) + "&actual=" + current::ToString(y);
                            const std::string img_src = img_a + "&bins=25&dim=250&nolegend";
                            html += "    <td><a href='" + img_a + "&bins=50'><img src='" + img_src + "' /></a></td>\n";
                          }
                          html += "  </tr>\n";
                        }
                        html += "</table>\n";
                        r(html, HTTPResponseCode.OK, current::net::constants::kDefaultHTMLContentType);
                      }
                    }) +
      http.Register(
          "/training_plots",
          [descriptive_optimization_accuracy_plot, discriminant_optimization_accuracy_plot](Request r) {
            using namespace current::gnuplot;
            GNUPlot plot;
            r(plot.Title("Classification Accuracy")
                  .Grid("back")
                  .XLabel("Iteration")
                  .YLabel("Classification accuracy")
                  .Plot(WithMeta([descriptive_optimization_accuracy_plot, discriminant_optimization_accuracy_plot](
                                     Plotter& p) {
                    size_t x = 0;
                    for (double y : descriptive_optimization_accuracy_plot) {
                      p(++x, y);
                    }
                  })
                            .Name("Descriptive training phase")
                            .Color("rgb 'blue'")
                            .LineWidth(2.5))
                  .Plot(WithMeta([descriptive_optimization_accuracy_plot, discriminant_optimization_accuracy_plot](
                                     Plotter& p) {
                    size_t x = descriptive_optimization_accuracy_plot.size();
                    for (double y : discriminant_optimization_accuracy_plot) {
                      p(++x, y);
                    }
                  })
                            .Name("Discriminant training phase")
                            .Color("rgb 'red'")
                            .LineWidth(2.5))
                  .OutputFormat("svg")
                  .ImageSize(800),
              HTTPResponseCode.OK,
              current::net::constants::kDefaultSVGContentType);
          });

  std::cout << "Starting the server on http://localhost:" << FLAGS_port << std::endl;

  http.Join();
}
