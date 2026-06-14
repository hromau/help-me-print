#include "filesystem_profile_store.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <optional>
#include <regex>
#include <sstream>

namespace duplexprint::platform {
namespace {

using duplexprint::core::OutputFace;
using duplexprint::core::PageOrder;
using duplexprint::core::PageParity;
using duplexprint::core::PrinterProfile;
using duplexprint::core::ProfileSource;
using duplexprint::core::ReloadMethod;

std::string escape_json(const std::string& value) {
  std::string output;
  output.reserve(value.size());
  for (const char ch : value) {
    switch (ch) {
      case '\\':
        output += "\\\\";
        break;
      case '"':
        output += "\\\"";
        break;
      case '\n':
        output += "\\n";
        break;
      default:
        output += ch;
        break;
    }
  }
  return output;
}

std::string output_face_to_string(const OutputFace value) {
  return value == OutputFace::Down ? "down" : "up";
}

std::string page_order_to_string(const PageOrder value) {
  return value == PageOrder::Reverse ? "reverse" : "normal";
}

std::string page_parity_to_string(const PageParity value) {
  return value == PageParity::Even ? "even" : "odd";
}

std::string reload_method_to_string(const ReloadMethod value) {
  switch (value) {
    case ReloadMethod::SameStack:
      return "same_stack";
    case ReloadMethod::FlipLongEdge:
      return "flip_long_edge";
    case ReloadMethod::FlipShortEdge:
      return "flip_short_edge";
  }
  return "same_stack";
}

std::string profile_source_to_string(const ProfileSource value) {
  switch (value) {
    case ProfileSource::Local:
      return "local";
    case ProfileSource::Cloud:
      return "cloud";
    case ProfileSource::Manual:
      return "manual";
  }
  return "local";
}

OutputFace parse_output_face(const std::string& value) {
  return value == "down" ? OutputFace::Down : OutputFace::Up;
}

PageOrder parse_page_order(const std::string& value) {
  return value == "reverse" ? PageOrder::Reverse : PageOrder::Normal;
}

PageParity parse_page_parity(const std::string& value) {
  return value == "even" ? PageParity::Even : PageParity::Odd;
}

ReloadMethod parse_reload_method(const std::string& value) {
  if (value == "flip_long_edge") {
    return ReloadMethod::FlipLongEdge;
  }
  if (value == "flip_short_edge") {
    return ReloadMethod::FlipShortEdge;
  }
  return ReloadMethod::SameStack;
}

ProfileSource parse_profile_source(const std::string& value) {
  if (value == "cloud") {
    return ProfileSource::Cloud;
  }
  if (value == "manual") {
    return ProfileSource::Manual;
  }
  return ProfileSource::Local;
}

std::optional<std::string> extract_string(const std::string& object, const std::string& field) {
  const std::regex pattern("\"" + field + "\"\\s*:\\s*\"([^\"]*)\"");
  std::smatch match;
  if (!std::regex_search(object, match, pattern) || match.size() < 2) {
    return std::nullopt;
  }
  return match[1].str();
}

std::optional<int> extract_int(const std::string& object, const std::string& field) {
  const std::regex pattern("\"" + field + "\"\\s*:\\s*(-?[0-9]+)");
  std::smatch match;
  if (!std::regex_search(object, match, pattern) || match.size() < 2) {
    return std::nullopt;
  }
  return std::stoi(match[1].str());
}

std::vector<std::string> extract_objects(const std::string& json) {
  std::vector<std::string> objects;
  std::size_t cursor = 0;
  while (true) {
    const std::size_t start = json.find('{', cursor);
    if (start == std::string::npos) {
      break;
    }
    std::size_t depth = 0;
    for (std::size_t index = start; index < json.size(); ++index) {
      if (json[index] == '{') {
        ++depth;
      } else if (json[index] == '}') {
        --depth;
        if (depth == 0) {
          objects.push_back(json.substr(start, index - start + 1));
          cursor = index + 1;
          break;
        }
      }
    }
    if (cursor <= start) {
      break;
    }
  }
  return objects;
}

std::optional<PrinterProfile> parse_profile(const std::string& object) {
  const auto printer_name = extract_string(object, "printerName");
  const auto manufacturer = extract_string(object, "manufacturer");
  const auto model = extract_string(object, "model");
  if (!printer_name || !manufacturer || !model) {
    return std::nullopt;
  }

  return PrinterProfile {
      .printer_name = *printer_name,
      .manufacturer = *manufacturer,
      .model = *model,
      .output_face = parse_output_face(extract_string(object, "outputFace").value_or("up")),
      .first_pass_parity = parse_page_parity(extract_string(object, "firstPassParity").value_or("odd")),
      .second_pass_parity = parse_page_parity(extract_string(object, "secondPassParity").value_or("even")),
      .first_pass_order = parse_page_order(extract_string(object, "firstPassOrder").value_or("normal")),
      .even_pages_order = parse_page_order(extract_string(object, "evenPagesOrder").value_or("normal")),
      .reload_method = parse_reload_method(extract_string(object, "reloadMethod").value_or("same_stack")),
      .confidence = extract_int(object, "confidence").value_or(0),
      .learned_at = extract_string(object, "learnedAt").value_or(""),
      .source = parse_profile_source(extract_string(object, "source").value_or("local")),
      .cloud_profile_id = extract_string(object, "cloudProfileId").value_or(""),
      .schema_version = extract_int(object, "schemaVersion").value_or(1),
  };
}

std::string serialize_profile(const PrinterProfile& profile) {
  std::ostringstream out;
  out << "    {\n";
  out << "      \"printerName\": \"" << escape_json(profile.printer_name) << "\",\n";
  out << "      \"manufacturer\": \"" << escape_json(profile.manufacturer) << "\",\n";
  out << "      \"model\": \"" << escape_json(profile.model) << "\",\n";
  out << "      \"outputFace\": \"" << output_face_to_string(profile.output_face) << "\",\n";
  out << "      \"firstPassParity\": \"" << page_parity_to_string(profile.first_pass_parity) << "\",\n";
  out << "      \"secondPassParity\": \"" << page_parity_to_string(profile.second_pass_parity) << "\",\n";
  out << "      \"firstPassOrder\": \"" << page_order_to_string(profile.first_pass_order) << "\",\n";
  out << "      \"evenPagesOrder\": \"" << page_order_to_string(profile.even_pages_order) << "\",\n";
  out << "      \"reloadMethod\": \"" << reload_method_to_string(profile.reload_method) << "\",\n";
  out << "      \"confidence\": " << profile.confidence << ",\n";
  out << "      \"learnedAt\": \"" << escape_json(profile.learned_at) << "\",\n";
  out << "      \"source\": \"" << profile_source_to_string(profile.source) << "\",\n";
  out << "      \"cloudProfileId\": \"" << escape_json(profile.cloud_profile_id) << "\",\n";
  out << "      \"schemaVersion\": " << profile.schema_version << "\n";
  out << "    }";
  return out.str();
}

}  // namespace

FilesystemProfileStore::FilesystemProfileStore(std::filesystem::path file_path)
    : file_path_(std::move(file_path)) {}

std::vector<PrinterProfile> FilesystemProfileStore::list() const {
  return read_profiles();
}

std::optional<PrinterProfile> FilesystemProfileStore::get(const std::string& printer_name) const {
  const auto profiles = read_profiles();
  const auto it = std::find_if(profiles.begin(), profiles.end(), [&](const PrinterProfile& profile) {
    return profile.printer_name == printer_name;
  });
  if (it == profiles.end()) {
    return std::nullopt;
  }
  return *it;
}

void FilesystemProfileStore::save(const PrinterProfile& profile) {
  auto profiles = read_profiles();
  const auto it = std::find_if(profiles.begin(), profiles.end(), [&](const PrinterProfile& existing) {
    return existing.printer_name == profile.printer_name;
  });
  if (it == profiles.end()) {
    profiles.push_back(profile);
  } else {
    *it = profile;
  }
  write_profiles(profiles);
}

std::vector<PrinterProfile> FilesystemProfileStore::read_profiles() const {
  if (!std::filesystem::exists(file_path_)) {
    return {};
  }

  std::ifstream input(file_path_);
  if (!input) {
    return {};
  }

  std::stringstream buffer;
  buffer << input.rdbuf();
  const auto objects = extract_objects(buffer.str());

  std::vector<PrinterProfile> profiles;
  for (const auto& object : objects) {
    if (const auto parsed = parse_profile(object)) {
      profiles.push_back(*parsed);
    }
  }
  return profiles;
}

void FilesystemProfileStore::write_profiles(const std::vector<PrinterProfile>& profiles) const {
  std::filesystem::create_directories(file_path_.parent_path());
  std::ofstream output(file_path_, std::ios::trunc);
  output << "{\n  \"profiles\": [\n";
  for (std::size_t index = 0; index < profiles.size(); ++index) {
    output << serialize_profile(profiles[index]);
    if (index + 1 < profiles.size()) {
      output << ",";
    }
    output << "\n";
  }
  output << "  ]\n}\n";
}

}  // namespace duplexprint::platform
