#ifndef SERVICES_H
#define SERVICES_H

#include <QString>

class QNetworkAccessManager;
class Neo4jHttpApiClient;

class Services
{
private:
    Services();
public:
    ~Services();
    static Services *instance();

    Services(const Services &) = delete;
    Services &operator =(const Services &) = delete;

    //!
    //! \return true iff successful
    //!
    bool initialize(QString *errorMsg = nullptr);

private:
    QNetworkAccessManager *networkAccessManager {nullptr};
    Neo4jHttpApiClient *neo4jHttpApiClient {nullptr};



};

#endif // SERVICES_H
