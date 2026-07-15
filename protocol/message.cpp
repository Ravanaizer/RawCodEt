#include "message.h"
#include <QJsonDocument>
#include <QtEndian>
#include <cstring>

// --- Serialization ---
// Serialize message to binary packet
QByteArray Message::serialize() const {
  QJsonObject msg;
  msg["flag"] = flag;
  msg["payload"] = payload;
  QByteArray body = QJsonDocument(msg).toJson(QJsonDocument::Compact);

  QByteArray packet;
  quint32 len = qToBigEndian<quint32>(static_cast<quint32>(body.size()));
  packet.append(reinterpret_cast<const char *>(&len), 4);
  packet.append(body);
  return packet;
}

// --- Deserialization ---
// Try to deserialize message from buffer
bool Message::tryDeserialize(QByteArray &buffer, Message &out) {
  // Need at least 4 bytes to read the length header
  if (buffer.size() < 4)
    return false;

  // Read length in big-endian format
  quint32 len;
  std::memcpy(&len, buffer.constData(), 4);
  len = qFromBigEndian<quint32>(len);

  // Wait until the full body is available
  if (buffer.size() < 4 + static_cast<int>(len))
    return false;

  // Extract the body and remove it from the buffer
  QByteArray body = buffer.mid(4, len);
  buffer.remove(0, 4 + len);

  // Parse JSON
  QJsonObject msg = QJsonDocument::fromJson(body).object();
  out.flag = msg["flag"].toString();
  out.payload = msg["payload"].toObject();
  return true;
}
