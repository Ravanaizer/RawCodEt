#pragma once

#include <QByteArray>
#include <QJsonObject>

class Message {
public:
  QString flag;
  QJsonObject payload;

  QByteArray serialize() const;

  static bool tryDeserialize(QByteArray &buffer, Message &out);
};
