#include "printer_identity.hpp"

#include <algorithm>
#include <cctype>

namespace duplexprint::platform {
namespace {

std::string trim(const std::string& value) {
  const auto first = std::find_if_not(value.begin(), value.end(), [](unsigned char ch) { return std::isspace(ch) != 0; });
  const auto last = std::find_if_not(value.rbegin(), value.rend(), [](unsigned char ch) { return std::isspace(ch) != 0; }).base();
  if (first >= last) {
    return {};
  }
  return std::string(first, last);
}

std::string normalize_token_stream(const std::string& value) {
  std::string normalized;
  normalized.reserve(value.size());

  bool last_was_space = false;
  for (const unsigned char raw : value) {
    if (std::isalnum(raw) != 0) {
      normalized.push_back(static_cast<char>(std::tolower(raw)));
      last_was_space = false;
      continue;
    }

    if (!last_was_space) {
      normalized.push_back(' ');
      last_was_space = true;
    }
  }

  return trim(normalized);
}

}  // namespace

std::string PrinterIdentity::normalize_label(const std::string& value) {
  return normalize_token_stream(value);
}

std::string PrinterIdentity::normalize_key(const std::string& manufacturer, const std::string& model) {
  const auto normalized_manufacturer = normalize_label(manufacturer);
  const auto normalized_model = normalize_label(model);
  if (normalized_manufacturer.empty()) {
    return normalized_model;
  }
  if (normalized_model.empty()) {
    return normalized_manufacturer;
  }
  return normalized_manufacturer + ":" + normalized_model;
}

cloud::PrinterFingerprint PrinterIdentity::fingerprint_for(const core::PrinterInfo& printer) {
  return cloud::PrinterFingerprint {
      .manufacturer = printer.manufacturer,
      .model = printer.model,
      .normalized_key = normalize_key(printer.manufacturer, printer.model),
      .device_name = printer.device_name,
  };
}

}  // namespace duplexprint::platform
