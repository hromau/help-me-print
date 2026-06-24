#include "workflow_engine.hpp"

#include <algorithm>
#include <charconv>
#include <string_view>

namespace duplexprint::core {
namespace {

std::vector<std::int32_t> page_range(const std::int32_t page_count, const PageParity parity) {
  std::vector<std::int32_t> pages;
  for (std::int32_t page = 1; page <= page_count; ++page) {
    const bool matches = (parity == PageParity::Odd && page % 2 == 1) ||
                         (parity == PageParity::Even && page % 2 == 0);
    if (matches) {
      pages.push_back(page);
    }
  }
  return pages;
}

PassPlan make_plan(
    const std::int32_t pass,
    std::string description,
    std::vector<std::int32_t> pages,
    const PageOrder order,
    const PageParity parity,
    const bool inserts_blank_trailing_sheet = false) {
  if (order == PageOrder::Reverse) {
    std::reverse(pages.begin(), pages.end());
  }

  return PassPlan {
      .pass = pass,
      .description = std::move(description),
      .pages = std::move(pages),
      .order = order,
      .parity = parity,
      .inserts_blank_trailing_sheet = inserts_blank_trailing_sheet,
  };
}

struct WorkflowResolution {
  PageParity first_pass_parity {PageParity::Odd};
  PageParity second_pass_parity {PageParity::Even};
  PageOrder first_pass_order {PageOrder::Normal};
  PageOrder second_pass_order {PageOrder::Normal};
};

WorkflowResolution resolve_workflow(const std::optional<PrinterProfile>& profile) {
  if (!profile.has_value()) {
    return {};
  }

  if (profile->reload_method == ReloadMethod::SameStack) {
    return WorkflowResolution {
        .first_pass_parity = PageParity::Even,
        .second_pass_parity = PageParity::Odd,
        .first_pass_order = PageOrder::Normal,
        .second_pass_order = PageOrder::Normal,
    };
  }

  return WorkflowResolution {
      .first_pass_parity = profile->first_pass_parity,
      .second_pass_parity = profile->second_pass_parity,
      .first_pass_order = profile->first_pass_order,
      .second_pass_order = profile->second_pass_parity == PageParity::Even ? profile->even_pages_order : PageOrder::Normal,
  };
}

int parse_int(const std::string_view value, const int fallback = 0) {
  int parsed = fallback;
  const auto* begin = value.data();
  const auto* end = value.data() + value.size();
  const auto result = std::from_chars(begin, end, parsed);
  if (result.ec != std::errc() || result.ptr != end) {
    return fallback;
  }
  return parsed;
}

OutputFace resolve_output_face(const int observed_top_sheet_page) {
  return observed_top_sheet_page != 0 && observed_top_sheet_page % 2 == 0 ? OutputFace::Down : OutputFace::Up;
}

}  // namespace

WorkflowSummary WorkflowEngine::create_workflow(
    const DocumentInfo& document,
    const PrinterInfo& printer,
    const std::optional<PrinterProfile>& profile) const {
  const auto odd_pages = page_range(document.page_count, PageParity::Odd);
  const auto even_pages = page_range(document.page_count, PageParity::Even);
  const auto workflow = resolve_workflow(profile);
  const auto& first_pass_pages = workflow.first_pass_parity == PageParity::Odd ? odd_pages : even_pages;
  const auto& second_pass_pages = workflow.second_pass_parity == PageParity::Odd ? odd_pages : even_pages;
  const bool inserts_blank_trailing_sheet = first_pass_pages.size() < second_pass_pages.size();

  return WorkflowSummary {
      .document = document,
      .printer = printer,
      .profile = profile,
      .first_pass = make_plan(
          1,
          "Print first pass.",
          first_pass_pages,
          workflow.first_pass_order,
          workflow.first_pass_parity,
          inserts_blank_trailing_sheet),
      .second_pass = make_plan(
          2,
          "Reload the paper stack, then print the remaining pages.",
          second_pass_pages,
          workflow.second_pass_order,
          workflow.second_pass_parity),
      .requires_learning = !profile.has_value(),
      .final_stack_goal = "page_1_on_top",
  };
}

WorkflowSummary WorkflowEngine::create_calibration_workflow(
    const DocumentInfo& document,
    const PrinterInfo& printer) const {
  return WorkflowSummary {
      .document = document,
      .printer = printer,
      .profile = std::nullopt,
      .first_pass = make_plan(1, "Print calibration even pages first.", page_range(document.page_count, PageParity::Even), PageOrder::Normal, PageParity::Even),
      .second_pass = make_plan(2, "Calibration second pass is not used.", {}, PageOrder::Normal, PageParity::Odd),
      .requires_learning = true,
      .final_stack_goal = "page_1_on_top",
  };
}

PassPlan WorkflowEngine::create_second_pass_plan(
    const DocumentInfo& document,
    const LearningAnswers& answers) const {
  const auto order = answers.reload_method == ReloadMethod::SameStack ? PageOrder::Reverse : PageOrder::Normal;
  return make_plan(
      2,
      "Reload the printed stack exactly as it came out, then print the remaining pages.",
      page_range(document.page_count, PageParity::Even),
      order,
      PageParity::Even);
}

DuplexComputation WorkflowEngine::learn_from_calibration(
    const std::string& observed_top_sheet_page) const {
  const int parsed = parse_int(observed_top_sheet_page, 0);
  const PageParity first_pass_parity = parsed <= 2 ? PageParity::Even : PageParity::Odd;
  const PageParity second_pass_parity = first_pass_parity == PageParity::Even ? PageParity::Odd : PageParity::Even;
  const PageOrder first_pass_order = first_pass_parity == PageParity::Even ? PageOrder::Normal : PageOrder::Reverse;
  const PageOrder even_pages_order = second_pass_parity == PageParity::Even ? PageOrder::Normal : PageOrder::Reverse;

  return DuplexComputation {
      .output_face = resolve_output_face(parsed),
      .first_pass_parity = first_pass_parity,
      .second_pass_parity = second_pass_parity,
      .first_pass_order = first_pass_order,
      .even_pages_order = even_pages_order,
      .reload_method = first_pass_parity == PageParity::Even ? ReloadMethod::SameStack : ReloadMethod::FlipLongEdge,
  };
}

DuplexComputation WorkflowEngine::learn_from_answers(const LearningAnswers& answers) const {
  const int parsed = parse_int(answers.observed_top_sheet_page, 1);
  const PageOrder even_pages_order = answers.reload_method == ReloadMethod::SameStack ? PageOrder::Reverse : PageOrder::Normal;
  const PageParity first_pass_parity = answers.reload_method == ReloadMethod::SameStack ? PageParity::Even : PageParity::Odd;
  const PageParity second_pass_parity = first_pass_parity == PageParity::Even ? PageParity::Odd : PageParity::Even;
  const PageOrder first_pass_order = first_pass_parity == PageParity::Even ? PageOrder::Normal : PageOrder::Reverse;

  return DuplexComputation {
      .output_face = resolve_output_face(parsed),
      .first_pass_parity = first_pass_parity,
      .second_pass_parity = second_pass_parity,
      .first_pass_order = first_pass_order,
      .even_pages_order = even_pages_order,
      .reload_method = answers.reload_method,
  };
}

}  // namespace duplexprint::core
