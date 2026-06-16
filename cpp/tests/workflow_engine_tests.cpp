#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include "filesystem_profile_store.hpp"
#include "printer_profile_resolver.hpp"
#include "test_profile_repository.hpp"
#include "workflow_engine.hpp"

using duplexprint::core::DocumentInfo;
using duplexprint::core::DuplexComputation;
using duplexprint::core::LearningAnswers;
using duplexprint::core::OutputFace;
using duplexprint::core::PageOrder;
using duplexprint::core::PageParity;
using duplexprint::core::PrinterInfo;
using duplexprint::core::PrinterProfile;
using duplexprint::core::PrinterTrainingState;
using duplexprint::core::ProfileSource;
using duplexprint::core::ReloadMethod;
using duplexprint::core::WorkflowEngine;

namespace {

int failures = 0;

void expect_true(const bool condition, const std::string& message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
    ++failures;
  }
}

template <typename T>
void expect_equal(const T& actual, const T& expected, const std::string& message) {
  if (!(actual == expected)) {
    std::cerr << "FAIL: " << message << '\n';
    ++failures;
  }
}

void expect_pages(const std::vector<std::int32_t>& actual, const std::vector<std::int32_t>& expected, const std::string& message) {
  if (actual != expected) {
    std::ostringstream stream;
    stream << message << " expected [";
    for (std::size_t i = 0; i < expected.size(); ++i) {
      stream << expected[i] << (i + 1 < expected.size() ? ", " : "");
    }
    stream << "] got [";
    for (std::size_t i = 0; i < actual.size(); ++i) {
      stream << actual[i] << (i + 1 < actual.size() ? ", " : "");
    }
    stream << "]";
    std::cerr << "FAIL: " << stream.str() << '\n';
    ++failures;
  }
}

PrinterInfo sample_printer() {
  return PrinterInfo {
      .id = "hp-580",
      .name = "HP Smart Tank 580",
      .device_name = "HP_Smart_Tank_580",
      .manufacturer = "HP",
      .model = "Smart Tank 580",
      .training_state = PrinterTrainingState::Untrained,
      .resolved_profile_source = std::nullopt,
  };
}

DocumentInfo sample_document(const int page_count = 8) {
  return DocumentInfo {
      .path = "/tmp/sample.pdf",
      .file_name = "sample.pdf",
      .page_count = page_count,
  };
}

PrinterProfile same_stack_profile() {
  return PrinterProfile {
      .printer_name = "HP Smart Tank 580",
      .manufacturer = "HP",
      .model = "Smart Tank 580",
      .normalized_key = "hp:smart tank 580",
      .output_face = OutputFace::Up,
      .first_pass_parity = PageParity::Even,
      .second_pass_parity = PageParity::Odd,
      .first_pass_order = PageOrder::Normal,
      .even_pages_order = PageOrder::Reverse,
      .reload_method = ReloadMethod::SameStack,
      .confidence = 98,
      .learned_at = "2026-06-09T00:00:00.000Z",
      .source = ProfileSource::Local,
      .cloud_profile_id = "",
      .schema_version = 1,
  };
}

PrinterProfile flip_profile() {
  return PrinterProfile {
      .printer_name = "HP Smart Tank 580",
      .manufacturer = "HP",
      .model = "Smart Tank 580",
      .normalized_key = "hp:smart tank 580",
      .output_face = OutputFace::Down,
      .first_pass_parity = PageParity::Odd,
      .second_pass_parity = PageParity::Even,
      .first_pass_order = PageOrder::Reverse,
      .even_pages_order = PageOrder::Normal,
      .reload_method = ReloadMethod::FlipLongEdge,
      .confidence = 98,
      .learned_at = "2026-06-09T00:00:00.000Z",
      .source = ProfileSource::Cloud,
      .cloud_profile_id = "cloud-1",
      .schema_version = 1,
  };
}

void test_default_workflow() {
  WorkflowEngine engine;
  const auto workflow = engine.create_workflow(sample_document(), sample_printer());

  expect_true(workflow.requires_learning, "missing profile should require learning");
  expect_pages(workflow.first_pass.pages, {1, 3, 5, 7}, "default first pass pages");
  expect_equal(workflow.first_pass.order, PageOrder::Normal, "default first pass order");
  expect_equal(workflow.first_pass.parity, PageParity::Odd, "default first pass parity");
  expect_pages(workflow.second_pass.pages, {2, 4, 6, 8}, "default second pass pages");
  expect_equal(workflow.second_pass.order, PageOrder::Normal, "default second pass order");
}

void test_same_stack_workflow() {
  WorkflowEngine engine;
  const auto workflow = engine.create_workflow(sample_document(5), sample_printer(), same_stack_profile());

  expect_true(!workflow.requires_learning, "known profile should skip learning");
  expect_pages(workflow.first_pass.pages, {2, 4}, "same-stack first pass pages");
  expect_true(workflow.first_pass.inserts_blank_trailing_sheet, "odd-page same-stack needs blank trailing sheet");
  expect_pages(workflow.second_pass.pages, {1, 3, 5}, "same-stack second pass pages");
}

void test_flip_workflow() {
  WorkflowEngine engine;
  const auto workflow = engine.create_workflow(sample_document(), sample_printer(), flip_profile());

  expect_pages(workflow.first_pass.pages, {7, 5, 3, 1}, "flip profile reverses first pass");
  expect_equal(workflow.second_pass.order, PageOrder::Normal, "flip profile keeps even second pass normal");
}

void test_learning() {
  WorkflowEngine engine;
  const DuplexComputation same_stack = engine.learn_from_calibration("2");
  expect_equal(same_stack.output_face, OutputFace::Down, "page 2 should imply output face down");
  expect_equal(same_stack.first_pass_parity, PageParity::Even, "page 2 should imply even-first workflow");
  expect_equal(same_stack.reload_method, ReloadMethod::SameStack, "page 2 should imply same-stack reload");

  const DuplexComputation flip = engine.learn_from_answers(LearningAnswers {
      .observed_top_sheet_page = "1",
      .reload_method = ReloadMethod::FlipLongEdge,
  });
  expect_equal(flip.first_pass_order, PageOrder::Reverse, "flip-long-edge keeps reverse first pass");
  expect_equal(flip.second_pass_parity, PageParity::Even, "flip-long-edge keeps even second pass");
}

void test_second_pass_plan() {
  WorkflowEngine engine;
  const auto plan = engine.create_second_pass_plan(sample_document(), LearningAnswers {
      .observed_top_sheet_page = "7",
      .reload_method = ReloadMethod::SameStack,
  });

  expect_equal(plan.pass, 2, "second pass plan should mark pass 2");
  expect_pages(plan.pages, {8, 6, 4, 2}, "same-stack second pass should reverse even pages");
  expect_equal(plan.order, PageOrder::Reverse, "same-stack second pass order");
}

void test_filesystem_profile_store() {
  const auto temp_dir = std::filesystem::temp_directory_path() / "duplexprint-cpp-tests";
  std::filesystem::create_directories(temp_dir);
  const auto file_path = temp_dir / "profiles.json";
  std::filesystem::remove(file_path);

  duplexprint::platform::FilesystemProfileStore store(file_path);
  store.save(same_stack_profile());

  const auto saved = store.get("HP", "Smart Tank 580", "HP Smart Tank 580");
  expect_true(saved.has_value(), "filesystem profile store should return saved profile");
  expect_equal(store.list().size(), static_cast<std::size_t>(1), "filesystem profile store list size");
  expect_equal(saved->confidence, 98, "filesystem profile store confidence");
}

void test_printer_profile_resolver() {
  const auto temp_dir = std::filesystem::temp_directory_path() / "duplexprint-cpp-tests";
  std::filesystem::create_directories(temp_dir);
  const auto file_path = temp_dir / "resolver-profiles.json";
  std::filesystem::remove(file_path);

  duplexprint::platform::FilesystemProfileStore store(file_path);
  duplexprint::tests::TestProfileRepository cloud_repo({
      {
          .profile = flip_profile(),
          .match_quality = duplexprint::core::PrinterMatchQuality::Normalized,
      },
  });

  duplexprint::platform::PrinterProfileResolver resolver(store, cloud_repo);
  const auto cloud_resolved = resolver.resolve(sample_printer());
  expect_equal(cloud_resolved.printer.training_state, PrinterTrainingState::TrainedCloud, "resolver should use cloud profile first");

  auto local_profile = same_stack_profile();
  local_profile.source = ProfileSource::Local;
  store.save(local_profile);

  const auto local_resolved = resolver.resolve(sample_printer());
  expect_equal(local_resolved.printer.training_state, PrinterTrainingState::TrainedLocal, "resolver should prefer local profile");
  expect_true(local_resolved.profile.has_value(), "resolver should return local profile");
}

}  // namespace

int main() {
  test_default_workflow();
  test_same_stack_workflow();
  test_flip_workflow();
  test_learning();
  test_second_pass_plan();
  test_filesystem_profile_store();
  test_printer_profile_resolver();

  if (failures != 0) {
    std::cerr << failures << " test(s) failed\n";
    return EXIT_FAILURE;
  }

  std::cout << "All DuplexPrint core tests passed\n";
  return EXIT_SUCCESS;
}
