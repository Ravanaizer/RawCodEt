#pragma once

#include <QByteArray>
#include <QJsonObject>

// Message class for client-server communication
class Message {
public:
  QString flag;
  QJsonObject payload;

  // Serialize message to binary packet
  QByteArray serialize() const;

  // Try to deserialize message from buffer
  static bool tryDeserialize(QByteArray &buffer, Message &out);
};
