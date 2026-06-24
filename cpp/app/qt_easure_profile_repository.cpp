#include "qt_easure_profile_repository.hpp"

#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <stdexcept>

namespace duplexprint::app {
namespace {

core::OutputFace output_face_from_json(const QString& value) {
  return value == "down" ? core::OutputFace::Down : core::OutputFace::Up;
}

core::PageParity parity_from_json(const QString& value) {
  return value == "even" ? core::PageParity::Even : core::PageParity::Odd;
}

core::PageOrder order_from_json(const QString& value) {
  return value == "reverse" ? core::PageOrder::Reverse : core::PageOrder::Normal;
}

core::ReloadMethod reload_from_json(const QString& value) {
  if (value == "flip_long_edge") {
    return core::ReloadMethod::FlipLongEdge;
  }
  if (value == "flip_short_edge") {
    return core::ReloadMethod::FlipShortEdge;
  }
  return core::ReloadMethod::SameStack;
}

QJsonObject profile_to_json(const core::PrinterProfile& profile) {
  return QJsonObject {
      {"displayName", QString::fromStdString(profile.printer_name)},
      {"manufacturer", QString::fromStdString(profile.manufacturer)},
      {"model", QString::fromStdString(profile.model)},
      {"outputFace", profile.output_face == core::OutputFace::Down ? "down" : "up"},
      {"firstPassParity", profile.first_pass_parity == core::PageParity::Even ? "even" : "odd"},
      {"secondPassParity", profile.second_pass_parity == core::PageParity::Even ? "even" : "odd"},
      {"firstPassOrder", profile.first_pass_order == core::PageOrder::Reverse ? "reverse" : "normal"},
      {"evenPagesOrder", profile.even_pages_order == core::PageOrder::Reverse ? "reverse" : "normal"},
      {"reloadMethod",
       profile.reload_method == core::ReloadMethod::FlipLongEdge
           ? "flip_long_edge"
           : profile.reload_method == core::ReloadMethod::FlipShortEdge ? "flip_short_edge" : "same_stack"},
      {"confidence", profile.confidence},
      {"votes", 1},
      {"createdAt", QString::fromStdString(profile.learned_at)},
  };
}

std::optional<core::PrinterProfile> profile_from_json(const QJsonObject& object) {
  if (object.isEmpty() || object.contains("error")) {
    return std::nullopt;
  }

  return core::PrinterProfile {
      .printer_name = object.value("displayName").toString().toStdString(),
      .manufacturer = object.value("manufacturer").toString().toStdString(),
      .model = object.value("model").toString().toStdString(),
      .normalized_key = object.value("normalizedKey").toString().toStdString(),
      .output_face = output_face_from_json(object.value("outputFace").toString()),
      .first_pass_parity = parity_from_json(object.value("firstPassParity").toString()),
      .second_pass_parity = parity_from_json(object.value("secondPassParity").toString()),
      .first_pass_order = order_from_json(object.value("firstPassOrder").toString()),
      .even_pages_order = order_from_json(object.value("evenPagesOrder").toString()),
      .reload_method = reload_from_json(object.value("reloadMethod").toString()),
      .confidence = object.value("confidence").toInt(0),
      .learned_at = object.value("updatedAt").toString().toStdString(),
      .source = core::ProfileSource::Cloud,
      .cloud_profile_id = object.value("normalizedKey").toString().toStdString(),
      .schema_version = object.value("schemaVersion").toInt(1),
  };
}

}  // namespace

QtEasureProfileRepository::QtEasureProfileRepository(QUrl base_url)
    : base_url_(std::move(base_url)) {}

std::optional<cloud::ResolvedProfile> QtEasureProfileRepository::find_profile(
    const cloud::PrinterFingerprint& fingerprint) const {
  QUrl url = base_url_;
  url.setPath(base_url_.path() + "/profiles/" +
              QString::fromUtf8(QUrl::toPercentEncoding(QString::fromStdString(fingerprint.manufacturer))) +
              "/" +
              QString::fromUtf8(QUrl::toPercentEncoding(QString::fromStdString(fingerprint.model))));

  QNetworkAccessManager manager;
  QNetworkRequest request(url);
  request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

  QEventLoop loop;
  auto* reply = manager.get(request);
  QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
  loop.exec();

  const auto status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
  const auto payload = reply->readAll();
  reply->deleteLater();

  if (status != 200) {
    return std::nullopt;
  }

  const auto document = QJsonDocument::fromJson(payload);
  const auto profile = profile_from_json(document.object());
  if (!profile) {
    return std::nullopt;
  }

  return cloud::ResolvedProfile {
      .profile = *profile,
      .match_quality = core::PrinterMatchQuality::Normalized,
  };
}

std::vector<cloud::ResolvedProfile> QtEasureProfileRepository::refresh_cached_profiles() {
  return {};
}

void QtEasureProfileRepository::save_learned_profile(const core::PrinterProfile& profile) {
  QUrl url = base_url_;
  url.setPath(base_url_.path() + "/profile-submissions/" +
              QString::fromUtf8(QUrl::toPercentEncoding(QString::fromStdString(profile.manufacturer))) +
              "/" +
              QString::fromUtf8(QUrl::toPercentEncoding(QString::fromStdString(profile.model))));

  QNetworkAccessManager manager;
  QNetworkRequest request(url);
  request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

  QEventLoop loop;
  auto* reply = manager.post(request, QJsonDocument(profile_to_json(profile)).toJson(QJsonDocument::Compact));
  QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
  loop.exec();

  const auto status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
  const auto payload = reply->readAll();
  const auto error = reply->errorString().toStdString();
  reply->deleteLater();

  if (status != 202) {
    throw std::runtime_error(
        "Failed to submit learned printer profile to Easure: " +
        (!payload.isEmpty() ? payload.toStdString() : error));
  }
}

}  // namespace duplexprint::app
