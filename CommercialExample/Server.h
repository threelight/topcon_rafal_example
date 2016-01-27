#ifndef FATIGUE_MANAGER_SERVER_H_
#define FATIGUE_MANAGER_SERVER_H_

#include <cstdint>
#include <vector>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/shared_array.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/strand.hpp>
#include <boost/thread.hpp>
#include "FatigueManager/Resources.h"
#include "FatigueManager/SessionManager.h"
#include "FatigueManager/AlertManager.h"
#include "FatigueManager1/SessionManager.h"
#include "Asio/SharedUdp/Acceptor.h"
#include "ServiceUtil/Config.h"
#include "ServiceUtil/LibInit.h"
#include "Ssl/SslInit.h"
#include "Ssl/DtlsContext.h"
#include "Asio/Generic/SocketFactory.h"
#include "Asio/Generic/Socket.h"

namespace FatigueManager
{

// Forward declarations
class Session;

class Server
{
public:
    /// Returns the server version, as a string
    static const std::string &version();

    /// Constructor, takes a string representing the DSN used to connect to
    /// the odbc database. Optionally takes an address and a port for the
    /// server to bind to
    Server(
        const std::string &dsn,
        const boost::asio::ip::address &address = boost::asio::ip::address(),
        std::uint16_t port = 0);

    /// Destructor, stops the service (waiting for all threads to finish),
    /// before exiting
    ~Server();

#ifndef FATIGUE_MANAGER_NO_ENCRYPTION

    /// Sets the encryption options for the application
    /// @preconditions
    ///     - application is not running
    void setEncryption(
        const char *caPEMFile,
        const char *privateKeyPEMFile,
        const char *certificatePEMFile);

#endif // FATIGUE_MANAGER_NO_ENCRYPTION

    /// Sets the notifcation url to use. Notification urls are of the
    /// form [protocol]://[address]. If the protocol is not supported by
    /// the application, this function will throw
    /// @preconditions
    ///     - application is not running
    void setNotificationUrl(const char *url);

    /// Starts the server running. If the server is already running, this
    /// function will do nothing.
    /// @threadsafety
    ///     - This function is safe to call from multiple threads
    /// @error
    ///     - Impplements the strong guarantee. If this function throws an
    ///     exception, it can be
    void run();

    /// Stops the server running. This function waits for any server threads
    /// to finish running. This function should not be called from the
    /// io_service that this server uses.
    /// @threadsafety
    ///     - This function is safe to call from an asynchronous context
    void stop();

    /// Waits for the server to finish
    void wait();

private:
    //@{
    /// Private copy constructor and assignment operator, not implemented
    Server(const Server &);
    Server &operator=(const Server &);
    //@}

    /// Starts listening for incoming connections from displays
    void startAccept();

    /// Handles an incoming connection from a display, starting the dtls
    /// handshake
    void handleAccept(
        SocketPtr socket,
        const boost::system::error_code &ec);

    /// Handles a dtls handshake with a display, starting the comms handshake
    void handleDtlsHandshake(
        SocketPtr socket,
        const boost::system::error_code &ec);

    /// Handles a handshake with a display, waiting for the first packet from
    /// the display
    void handleHandshake(
        SocketPtr socket,
        const boost::system::error_code &ec);

    /// Handles a old packet from the display, waiting for the first packet
    void handleOldPacket(
        SocketPtr socket,
        boost::shared_array<std::uint8_t> bufferPtr,
        const boost::system::error_code &ec,
        std::size_t size);

    /// Handles the alarm update timer expiring, updates the session manager
    /// to check for alarm rule updates
    void handleAlarmRuleUpdate(const boost::system::error_code &ec);

    /// Handles the firmware update timer expiring, updates the session manager
    /// to check for firmware status updates
    void handleFirmwareUpdate(const boost::system::error_code &ec);

    /// Handles the alert where all plants have been offline for a configurable
    /// period of time
    void handleServerTimeout();

    /// Handles the alert where a plant has unexpectedly gone offline for a
    /// configurable period of time
    void handlePlantOfflineTimeout(const Sql::IdType &plantDeviceId);

    /// Handles the alert where the plant has received an alarm
    void handleAlarmNotification(const Sql::IdType &plantDeviceId);

    /// Sends out any notifications
    /// @preconditions
    ///     - notificationSocket_ is not null
    void beginNotification();

    /// Handles writes to the notification socket
    void handleNotificationSend(const boost::system::error_code &ec);

    /// Cancels all outstanding asynchronous operations
    void cancelAsync();

    // Library initialisation required by the fatigue manager
    ServiceUtil::OtlInit otlInit_;
    ServiceUtil::LogInit logInit_;

#ifndef FATIGUE_MANAGER_NO_ENCRYPTION

    Ssl::SslInit sslInit_;

#endif // FATIGUE_MANAGER_NO_ENCRYPTION


    // Private data
    boost::asio::io_service service_;
    boost::asio::ip::udp::endpoint address_;
    boost::asio::ip::udp::endpoint remoteAddress_;
    std::string notificationType_;
    std::string notificationAddress_;

#ifndef FATIGUE_MANAGER_NO_ENCRYPTION

    Ssl::DtlsContext context_;

#endif // FATIGUE_MANAGER_NO_ENCRYPTION

    Resources resources_;
    std::unique_ptr<AlertManager> alertManager_;
    std::unique_ptr<SessionManager> sessionManager_;
    std::unique_ptr<FatigueManager1::SessionManager> sessionManager1_;
    Asio::SharedUdp::Acceptor acceptor_;
    std::size_t threadPoolSize_;
    ServiceUtil::Config config_;

    boost::asio::deadline_timer alarmRuleUpdateTimer_;
    boost::asio::deadline_timer firmwareUpdateTimer_;

    // The mutex is used to provide protection to the run and stop functions.
    // The condition variable is used to notify when the service has changed
    // state
    boost::try_mutex mutex_;
    boost::condition_variable cond_;
    std::unique_ptr<boost::thread_group> threads_;
    bool running_;

    /// Used for synchronising access to the notification socket
    boost::asio::io_service::strand notificationStrand_;
    boost::scoped_ptr<Asio::Generic::Socket> notificationSocket_;
    std::list<Sql::IdType> notificationAlarms_;
    std::vector<std::uint8_t> notificationBuffer_;
    bool notificationInProgress_;
};

} // FatigueManager namespace

#endif // FATIGUE_MANAGER_SERVER_H_
