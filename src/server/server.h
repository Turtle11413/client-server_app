#ifndef SERVER_H_
#define SERVER_H_

#include <QFile>
#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QVector>

class Server : public QObject {
  Q_OBJECT

public:
  Server(const int &port);
  ~Server();

signals:
  void NewFileUploaded(const QString &filename, const QString &load_time);

private slots:
  void NewConnection();

  bool isExistsFile(const QString &filename);
  void ReadMessageFromClient();
  void ReceiveFileFromClient(QDataStream &in, QTcpSocket *client);
  void SendFileToClient(const QString &filename, QTcpSocket *client);

  void UpdateClientTable(const QString &filename, const QString &load_time);
  void ClientDisconnected();

private:
  void FillFileMap();
  void LoadTable(QTcpSocket *client);

  QTcpServer *server_;
  QSet<QTcpSocket *> client_sockets_;

  QMap<QString, QString> file_map_;
};

#endif // SERVER_H_
