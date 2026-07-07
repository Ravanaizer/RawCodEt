#include "message.h"

#include <QJsonDocument>
#include <QtEndian>

QByteArray Message::serialize() const {
  QJsonObject msg;
  msg["flag"] = flag;
  msg["payload"] = payload;
  QByteArray body = QJsonDocument(msg).toJson(QJsonDocument::Compact);

  QByteArray packet;
  quint32 len = qToBigEndian<quint32>(body.size());
  packet.append(reinterpret_cast<const char*>(&len), 4);
  packet.append(body);
  return packet;
}


bool Message::tryDeserialize(QByteArray &buffer, Message &out) {
  if (buffer.size() < 4) return false;

  quint32 len;
  memcpy(&len, buffer.constData(), 4);
  len = qFromBigEndian<quint32>(len);

  if (buffer.size() < 4 + (int)len) return false;  // ждём остальное

  QByteArray body = buffer.mid(4, len);
  buffer.remove(0, 4 + len);

  QJsonObject msg = QJsonDocument::fromJson(body).object();
  out.flag = msg["flag"].toString();
  out.payload = msg["payload"].toObject();
  return true;
}
