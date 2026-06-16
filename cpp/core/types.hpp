#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace duplexprint::core {

enum class OutputFace {
  Up,
  Down
};

enum class PageOrder {
  Normal,
  Reverse
};

enum class ReloadMethod {
  SameStack,
  FlipLongEdge,
  FlipShortEdge
};

enum class PageParity {
  Odd,
  Even
};

enum class ProfileSource {
  Local,
  Cloud,
  Manual
};

enum class PrinterTrainingState {
  Untrained,
  TrainedLocal,
  TrainedCloud,
  TrainedCloudOverriddenLocal
};

enum class PrinterMatchQuality {
  Exact,
  Normalized,
  Heuristic
};

struct PrinterProfile {
  std::string printer_name;
  std::string manufacturer;
  std::string model;
  std::string normalized_key;
  OutputFace output_face {OutputFace::Up};
  PageParity first_pass_parity {PageParity::Odd};
  PageParity second_pass_parity {PageParity::Even};
  PageOrder first_pass_order {PageOrder::Normal};
  PageOrder even_pages_order {PageOrder::Normal};
  ReloadMethod reload_method {ReloadMethod::SameStack};
  std::int32_t confidence {0};
  std::string learned_at;
  ProfileSource source {ProfileSource::Local};
  std::string cloud_profile_id;
  std::int32_t schema_version {1};
};

struct PrinterInfo {
  std::string id;
  std::string name;
  std::string device_name;
  std::string manufacturer;
  std::string model;
  PrinterTrainingState training_state {PrinterTrainingState::Untrained};
  std::optional<ProfileSource> resolved_profile_source;
};

struct DocumentInfo {
  std::string path;
  std::string file_name;
  std::int32_t page_count {0};
};

struct PassPlan {
  std::int32_t pass {1};
  std::string description;
  std::vector<std::int32_t> pages;
  PageOrder order {PageOrder::Normal};
  PageParity parity {PageParity::Odd};
  bool inserts_blank_trailing_sheet {false};
};

struct WorkflowSummary {
  DocumentInfo document;
  PrinterInfo printer;
  std::optional<PrinterProfile> profile;
  PassPlan first_pass;
  PassPlan second_pass;
  bool requires_learning {true};
  std::string final_stack_goal {"page_1_on_top"};
};

struct CalibrationSummary {
  std::string calibration_pdf_path;
  WorkflowSummary workflow;
};

struct LearningAnswers {
  std::string observed_top_sheet_page;
  ReloadMethod reload_method {ReloadMethod::SameStack};
};

struct DuplexComputation {
  OutputFace output_face {OutputFace::Up};
  PageParity first_pass_parity {PageParity::Odd};
  PageParity second_pass_parity {PageParity::Even};
  PageOrder first_pass_order {PageOrder::Normal};
  PageOrder even_pages_order {PageOrder::Normal};
  ReloadMethod reload_method {ReloadMethod::SameStack};
};

}  // namespace duplexprint::core
