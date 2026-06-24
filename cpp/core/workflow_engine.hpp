#pragma once

#include "types.hpp"

namespace duplexprint::core {

class WorkflowEngine {
 public:
  [[nodiscard]] WorkflowSummary create_workflow(
      const DocumentInfo& document,
      const PrinterInfo& printer,
      const std::optional<PrinterProfile>& profile = std::nullopt) const;

  [[nodiscard]] WorkflowSummary create_calibration_workflow(
      const DocumentInfo& document,
      const PrinterInfo& printer) const;

  [[nodiscard]] PassPlan create_second_pass_plan(
      const DocumentInfo& document,
      const LearningAnswers& answers) const;

  [[nodiscard]] DuplexComputation learn_from_calibration(
      const std::string& observed_top_sheet_page) const;

  [[nodiscard]] DuplexComputation learn_from_answers(
      const LearningAnswers& answers) const;
};

}  // namespace duplexprint::core
