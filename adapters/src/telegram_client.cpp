#include "vertel/adapters/telegram/telegram_client.hpp"

#if VERTEL_HAS_LIBCURL
#include <curl/curl.h>
#else
#include <windows.h>
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")
#endif

#include <algorithm>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

#include "nlohmann/json.hpp"

// MSVC: curl/curl.h pulls in windows.h which re-defines SendMessage.
#ifdef SendMessage
#undef SendMessage
#endif

namespace vertel::adapters::telegram {
namespace {

constexpr const char *kTelegramApiBase = "https://api.telegram.org";

#if VERTEL_HAS_LIBCURL
size_t WriteBody(char *ptr, size_t size, size_t nmemb, void *userdata) {
  auto *out = static_cast<std::string *>(userdata);
  out->append(ptr, size * nmemb);
  return size * nmemb;
}
#endif

} // namespace

TelegramClient::TelegramClient(bool inject_sample_update)
    : inject_sample_update_(inject_sample_update) {}

TelegramClient::TelegramClient(std::string bot_token, int long_poll_timeout_seconds,
                               int request_timeout_seconds)
    : bot_token_(std::move(bot_token)),
      long_poll_timeout_seconds_(std::max(1, long_poll_timeout_seconds)),
      request_timeout_seconds_(std::max(5, request_timeout_seconds)) {}

TelegramClient::HttpResponse TelegramClient::PostForm(const std::string &endpoint,
                                                      const std::string &form_body) const {
#if !VERTEL_HAS_LIBCURL
  HINTERNET hSession = WinHttpOpen(L"VerTel-Bot/1.0",
      WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
      WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
  if (!hSession) throw std::runtime_error("WinHttpOpen failed");

  HINTERNET hConnect = WinHttpConnect(hSession,
      L"api.telegram.org", INTERNET_DEFAULT_HTTPS_PORT, 0);
  if (!hConnect) { WinHttpCloseHandle(hSession); throw std::runtime_error("WinHttpConnect failed"); }

  std::wstring path = L"/bot";
  std::wstring wtoken(bot_token_.begin(), bot_token_.end());
  std::wstring wendpoint(endpoint.begin(), endpoint.end());
  path += wtoken + L"/" + wendpoint;

  HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST", path.c_str(),
      nullptr, WINHTTP_NO_REFERER,
      WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
  if (!hRequest) {
      WinHttpCloseHandle(hConnect);
      WinHttpCloseHandle(hSession);
      throw std::runtime_error("WinHttpOpenRequest failed");
  }

  // Set timeout
  WinHttpSetTimeouts(hRequest, 10000, 10000, request_timeout_seconds_ * 1000, request_timeout_seconds_ * 1000);

  std::wstring headers = L"Content-Type: application/x-www-form-urlencoded\r\n";
  if (!WinHttpSendRequest(hRequest, headers.c_str(), -1L,
          (LPVOID)form_body.c_str(), (DWORD)form_body.size(), (DWORD)form_body.size(), 0)) {
      WinHttpCloseHandle(hRequest);
      WinHttpCloseHandle(hConnect);
      WinHttpCloseHandle(hSession);
      throw std::runtime_error("WinHttpSendRequest failed");
  }

  if (!WinHttpReceiveResponse(hRequest, nullptr)) {
      WinHttpCloseHandle(hRequest);
      WinHttpCloseHandle(hConnect);
      WinHttpCloseHandle(hSession);
      throw std::runtime_error("WinHttpReceiveResponse failed");
  }

  DWORD statusCode = 0;
  DWORD statusCodeSize = sizeof(statusCode);
  WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                      WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &statusCodeSize, WINHTTP_NO_HEADER_INDEX);

  std::string response_body;
  DWORD bytesAvailable = 0;
  while (WinHttpQueryDataAvailable(hRequest, &bytesAvailable) && bytesAvailable > 0) {
      std::vector<char> buffer(bytesAvailable + 1, 0);
      DWORD bytesRead = 0;
      if (WinHttpReadData(hRequest, buffer.data(), bytesAvailable, &bytesRead)) {
          response_body.append(buffer.data(), bytesRead);
      }
  }

  WinHttpCloseHandle(hRequest);
  WinHttpCloseHandle(hConnect);
  WinHttpCloseHandle(hSession);

  if (statusCode >= 400) {
      std::ostringstream oss;
      oss << "telegram http status " << statusCode << " body: " << response_body;
      throw std::runtime_error(oss.str());
  }

  return HttpResponse{.status_code = static_cast<long>(statusCode), .body = std::move(response_body)};
#else
  CURL *curl = curl_easy_init();
  if (curl == nullptr) {
    throw std::runtime_error("curl_easy_init failed");
  }

  std::string response_body;
  const std::string url = std::string(kTelegramApiBase) + "/bot" + bot_token_ + "/" + endpoint;

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_POST, 1L);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, form_body.c_str());
  curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(form_body.size()));
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteBody);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_body);
  curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, static_cast<long>(request_timeout_seconds_));
  curl_easy_setopt(curl, CURLOPT_USERAGENT, "vertel-bot/1.0");

  const CURLcode code = curl_easy_perform(curl);
  if (code != CURLE_OK) {
    const std::string error = curl_easy_strerror(code);
    curl_easy_cleanup(curl);
    throw std::runtime_error("telegram http error: " + error);
  }

  long status_code = 0;
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status_code);
  curl_easy_cleanup(curl);

  if (status_code >= 400) {
    std::ostringstream oss;
    oss << "telegram http status " << status_code;
    throw std::runtime_error(oss.str());
  }

  return HttpResponse{.status_code = status_code, .body = std::move(response_body)};
#endif
}

std::vector<vertel::core::Update> TelegramClient::ParseUpdates(const std::string &json) const {
  nlohmann::json payload;
  try {
    payload = nlohmann::json::parse(json);
  } catch (const nlohmann::json::parse_error &ex) {
    throw std::runtime_error(std::string("telegram response parse error: ") + ex.what());
  }

  if (!payload.value("ok", false)) {
    throw std::runtime_error("telegram response not ok");
  }

  const auto result_it = payload.find("result");
  if (result_it == payload.end() || !result_it->is_array()) {
    return {};
  }

  std::vector<vertel::core::Update> updates;
  for (const auto &item : *result_it) {
    try {
      if (!item.is_object()) {
        continue;
      }
      const auto message_it = item.find("message");
      if (message_it == item.end() || !message_it->is_object()) {
        continue;
      }
      const auto chat_it = message_it->find("chat");
      if (chat_it == message_it->end() || !chat_it->is_object()) {
        continue;
      }
      const auto text_it = message_it->find("text");
      if (text_it == message_it->end() || !text_it->is_string()) {
        continue;
      }

      const auto update_id_it = item.find("update_id");
      const auto chat_id_it = chat_it->find("id");
      if (update_id_it == item.end() || chat_id_it == chat_it->end() ||
          !update_id_it->is_number_integer() || !chat_id_it->is_number_integer()) {
        continue;
      }

      updates.push_back(vertel::core::Update{.update_id = update_id_it->get<std::int64_t>(),
                                             .chat_id = chat_id_it->get<std::int64_t>(),
                                             .text = text_it->get<std::string>()});
    } catch (const nlohmann::json::exception &) {
      continue;
    }
  }

  return updates;
}

std::string TelegramClient::UrlEncode(const std::string &value) {
#if !VERTEL_HAS_LIBCURL
  std::ostringstream escaped;
  escaped.fill('0');
  escaped << std::hex;
  for (unsigned char c : value) {
      if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
          escaped << c;
      } else {
          escaped << std::uppercase << '%' << std::setw(2) << int((unsigned char)c);
      }
  }
  return escaped.str();
#else
  CURL *curl = curl_easy_init();
  if (curl == nullptr) {
    throw std::runtime_error("curl_easy_init failed for encode");
  }
  char *escaped = curl_easy_escape(curl, value.c_str(), static_cast<int>(value.size()));
  if (escaped == nullptr) {
    curl_easy_cleanup(curl);
    throw std::runtime_error("curl_easy_escape failed");
  }
  std::string encoded(escaped);
  curl_free(escaped);
  curl_easy_cleanup(curl);
  return encoded;
#endif
}

std::vector<vertel::core::Update> TelegramClient::PollUpdates() {
  if (inject_sample_update_ && !sample_emitted_) {
    sample_emitted_ = true;
    return {vertel::core::Update{.update_id = 1, .chat_id = 1001, .text = "/start"}};
  }
  if (bot_token_.empty()) {
    return {};
  }

  std::ostringstream fields;
  fields << "timeout=" << long_poll_timeout_seconds_ << "&allowed_updates=%5B%22message%22%5D";
  if (next_update_offset_ > 0) {
    fields << "&offset=" << next_update_offset_;
  }

  const auto response = PostForm("getUpdates", fields.str());
  auto updates = ParseUpdates(response.body);
  for (const auto &update : updates) {
    next_update_offset_ = std::max(next_update_offset_, update.update_id + 1);
  }
  return updates;
}

void TelegramClient::SendMessage(const vertel::core::OutgoingMessage &message) {
  sent_messages_.push_back(message);
  if (inject_sample_update_) {
    return;
  }
  if (bot_token_.empty()) {
    throw std::runtime_error("TELEGRAM_BOT_TOKEN is required");
  }

  std::ostringstream fields;
  fields << "chat_id=" << message.chat_id << "&text=" << UrlEncode(message.text);
  (void)PostForm("sendMessage", fields.str());
}

const std::vector<vertel::core::OutgoingMessage> &TelegramClient::SentMessages() const {
  return sent_messages_;
}

} // namespace vertel::adapters::telegram
