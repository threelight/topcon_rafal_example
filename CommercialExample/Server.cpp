#include "FatigueManager/Server.h"
#include <iomanip>
#include <iterator>
#include <boost/bind.hpp>
#include <boost/asio/ip/basic_endpoint.hpp>
#include <boost/thread/thread.hpp>
#include <boost/exception_ptr.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/date_time/posix_time/time_formatters.hpp>
#include <boost/lexical_cast.hpp>
#include "FatigueManager/Sql.h"
#include "FatigueManager/Session.h"
#include "ServiceUtil/ServiceUtil.h"
#include "ServiceUtil/Log.h"
#include "ServiceUtil/Sql.h"
#include "ServiceUtil/Email.h"
#include "SmartCap/PacketType.h"
#include "Util/scopeguard.h"
#include "Util/IterPack.h"
#include "Asio/Comms/Error.h"
#include "otl.h"

namespace
{
// Private declarations

namespace Log = ServiceUtil::Log;

/// Time between alarm updates in mins
const std::size_t AlarmRuleUpdateIntervalMinutes = 10;

/// Time between firmware update checks, in minutes
const std::size_t FirmwareUpdateIntervalMinutes = 1;

//@{
/// Configuration constants
const std::string SmtpServerConfig = "mail_relay";
const std::string MailSenderConfig = "mail_sender";
const std::string ServerCommsRecipientConfig = "server_comms_recipient";
const std::string ServerCommsSubjectConfig = "server_comms_subject";
const std::string ServerCommsBodyConfig = "server_comms_body";
const std::string ServerCommsTimeoutConfig = "server_comms_timeout";
const std::string PlantOfflineRecipientConfig = "plant_offline_recipient";
const std::string PlantOfflineSubjectConfig = "plant_offline_subject";
const std::string PlantOfflineBodyConfig = "plant_offline_body";
const std::string PlantOfflineTimeoutConfig = "plant_offline_timeout";

const std::string OldPlantOfflineStatusTimeoutConfig = "plant_offline_status_timeout";

//@}

/// Default configuration constants
const unsigned int DefaultServerOfflineTimeout = 0;
const unsigned int DefaultPlantOfflineTimeout = 0;
const unsigned int DefaultOldPlantOfflineStatusTimeout = 0;

//@{
/// This constant is replaced with a string representation of the plant in the
/// subject and body of the mail message
const std::string MailSubst = "PLANTID";
//@}

/// Version string
const std::string Version = "0.0.0";

//@{
/// Filter for bytes arriving from old clients
const std::uint8_t OldPacketFilter[] = { 0x02, 0x00, 0x51, 0x47 };
const std::uint8_t OldPacketFilterMask[] = { 0xFF, 0x00, 0xFF, 0xFF };
//@}

///
const std::size_t OldPacketSize = 128;

/// type/address split token for the notification url
const std::string UrlSplitToken("://");

/// Socket factories
Asio::Generic::SocketFactory::Registry socketRegistry_;
Asio::Generic::UdpSocketFactory udpFactory_(socketRegistry_);
Asio::Generic::LocalSocketFactory localFactory_(socketRegistry_);

/// Current alarm notification packet information
const std::uint16_t AlarmNotificationType = 0x0000u;

/// Utility function to get the a value from a map, providing a default if the
/// value does not exist. The map is a string string map, so non-string values
/// will be lexically cast to the correct type
template <typename TType>
TType defaultAt(
    const ServiceUtil::Config &config,
    const std::string &key,
    const TType &defaultVal);

// Interface definitions

template <typename TType>
TType defaultAt(
    const ServiceUtil::Config &config,
    const std::string &key,
    const TType &defaultVal)
{
    ServiceUtil::Config::const_iterator iter;
    if ((iter = config.find(key)) != config.end() &&
        !iter->second.empty())
        return boost::lexical_cast<TType>(iter->second);

    return defaultVal;
}

} // Private namespace

// Interface definitions

namespace FatigueManager
{

const std::string &Server::version()
{
    return Version;
}

Server::Server(
    const std::string &dsn,
    const boost::asio::ip::address &address,
    std::uint16_t port)
: address_(address, port),

#ifndef FATIGUE_MANAGER_NO_ENCRYPTION

  context_(Ssl::DtlsContext::DefaultVerificationMode, 0),

#endif // FATIGUE_MANAGER_NO_ENCRYPTION

  resources_(service_, DatabasePolicy(dsn)),
  acceptor_(service_),
  threadPoolSize_(boost::thread::hardware_concurrency() ?
    boost::thread::hardware_concurrency(): 1),
  alarmRuleUpdateTimer_(service_),
  firmwareUpdateTimer_(service_),
  running_(false),
  notificationStrand_(service_)
{
    Log::info("Fatigue Manager Initialised (%s)", Version);
}

Server::~Server()
{
    try
    {
        stop();
    }
    catch (...)
    {
    }
}

#ifndef FATIGUE_MANAGER_NO_ENCRYPTION

void Server::setEncryption(
    const char *caPEMFile,
    const char *privateKeyPEMFile,
    const char *certificatePEMFile)
{
    context_.setCAFile(caPEMFile);
    context_.setLocalCertificate(certificatePEMFile, SSL_FILETYPE_PEM);
    context_.setPrivateKey(privateKeyPEMFile, SSL_FILETYPE_PEM);
}

#endif // FATIGUE_MANAGER_NO_ENCRYPTION

void Server::setNotificationUrl(const char *url)
{
    if (!url)
        throw std::runtime_error("Invalid url");

    // Split our url into type/address parts
    const char *end = url + std::strlen(url);
    auto iter = std::search(url, end, UrlSplitToken.begin(), UrlSplitToken.end());

    if (iter == end)
        throw std::runtime_error("Invalid url");

    std::string type(url, iter), address(iter + UrlSplitToken.size(), end);
    std::swap(type, notificationType_);
    std::swap(address, notificationAddress_);
}

void Server::run()
{
    // Dont do anything if we are already running
    boost::unique_lock<boost::try_mutex> lock(mutex_, boost::try_to_lock);
    if (!lock.owns_lock() || running_)
        return;

    try
    {
        service_.reset();

        // Read and verify our configuration from our database
        {
            config_.clear();
            DatabasePool::ResourcePtr sql = resources_.databasePool().acquire();
            Sql::init(*sql);
            ServiceUtil::Sql::readConfig(*sql, config_);
            ServiceUtil::verifyConfig(config_);
        }

        // Get our configuration variables
        unsigned int serverTimeout = defaultAt<unsigned int>(config_,
            ServerCommsTimeoutConfig, DefaultServerOfflineTimeout);
        unsigned int plantOfflineTimeout = defaultAt<unsigned int>(config_,
            PlantOfflineTimeoutConfig, DefaultPlantOfflineTimeout);
        unsigned int oldPlantOfflineTimeout = defaultAt<unsigned int>(config_,
            OldPlantOfflineStatusTimeoutConfig, DefaultOldPlantOfflineStatusTimeout);

        alertManager_.reset(new AlertManager(service_, resources_.databasePool()));
        if (serverTimeout)
        {
            alertManager_->setServerTimeout(
                boost::posix_time::minutes(serverTimeout),
                boost::bind(&Server::handleServerTimeout, this));
        }

        if (plantOfflineTimeout)
        {
            alertManager_->setPlantOfflineTimeout(
                boost::posix_time::minutes(plantOfflineTimeout),
                boost::bind(&Server::handlePlantOfflineTimeout, this, _1));
        }

        // Create our session managers
        sessionManager_.reset(new SessionManager(resources_));
        sessionManager1_.reset(new FatigueManager1::SessionManager(
            resources_.databasePool()));

        if (oldPlantOfflineTimeout)
            sessionManager1_->setTimeoutPeriod(
                boost::posix_time::minutes(oldPlantOfflineTimeout));

        if (serverTimeout)
        {
            sessionManager_->setSessionCountNotify(
                boost::bind(&AlertManager::notifySessionCount, alertManager_.get(),
                    AlertManager::NormalSession, _1));
            sessionManager1_->setSessionCountNotify(
                boost::bind(&AlertManager::notifySessionCount, alertManager_.get(),
                    AlertManager::OldSession, _1));
        }

        if (plantOfflineTimeout)
        {
            sessionManager_->setPlantOfflineNotify(
                boost::bind(&AlertManager::notifyPlantOffline,
                    alertManager_.get(), _1));
            sessionManager1_->setPlantOfflineNotify(
                boost::bind(&AlertManager::notifyPlantOffline,
                    alertManager_.get(), _1));
        }

        sessionManager_->setAlarmEventNotify(
            notificationStrand_.wrap(boost::bind(&Server::handleAlarmNotification,
                this, _1)));

        sessionManager1_->setAlarmEventNotify(
            notificationStrand_.wrap(boost::bind(&Server::handleAlarmNotification,
                this, _1)));

        // Start our udp acceptor waiting for connections
        acceptor_.open(address_.protocol());
        acceptor_.bind(address_);
        startAccept();

        // Start our update timers
        alarmRuleUpdateTimer_.expires_from_now(
            boost::posix_time::minutes(AlarmRuleUpdateIntervalMinutes));
        alarmRuleUpdateTimer_.async_wait(
            boost::bind(&Server::handleAlarmRuleUpdate, this, _1));

        firmwareUpdateTimer_.expires_from_now(
            boost::posix_time::minutes(FirmwareUpdateIntervalMinutes));
        firmwareUpdateTimer_.async_wait(
            boost::bind(&Server::handleFirmwareUpdate, this, _1));
    }
    catch (std::exception &e)
    {
        Log::error("Error Starting: %s", std::string(e.what()));
        cancelAsync();
        sessionManager_.reset();
        sessionManager1_.reset();
        alertManager_.reset();
        throw;
    }
    catch (otl_exception &e)
    {
        Log::error("Error Starting: %s", std::string((char *) e.msg));
        cancelAsync();
        sessionManager_.reset();
        sessionManager1_.reset();
        alertManager_.reset();
        throw;
    }

    // Start our threads to service our reads
    boost::exception_ptr exception;

    try
    {
        threads_.reset(new boost::thread_group);

        for (std::size_t index = 0; index < threadPoolSize_; ++index)
            threads_->create_thread(boost::bind(&ServiceUtil::runIoService,
                boost::ref(service_)));
    }
    catch (std::exception &e)
    {
        // There was an error creating our threads
        Log::error("Error Starting Threads: %s", std::string(e.what()));
        exception = boost::current_exception();
    }
    catch (...)
    {
        Log::error("Unknown Error Starting Threads.");
        exception = boost::current_exception();
    }

    if (exception)
    {
        // Wait for all threads in the pool to exit.
        service_.stop();
        threads_->join_all();
        cancelAsync();
        acceptor_.close();
        resources_.clear();
        sessionManager_.reset();
        sessionManager1_.reset();
        alertManager_.reset();
        boost::rethrow_exception(exception);
    }

    Log::info("Fatigue Manager Running");
    running_ = true;
}

void Server::stop()
{
    boost::lock_guard<boost::try_mutex> lock(mutex_);
    if (running_)
    {
        Log::info("Fatigue Manager Stopping (%s)", Version);
        service_.stop();
        threads_->join_all();
        cancelAsync();
        acceptor_.close();
        resources_.clear();
        sessionManager_.reset();
        sessionManager1_.reset();
        alertManager_.reset();
        running_ = false;
    }

    cond_.notify_all();
}

void Server::wait()
{
    boost::unique_lock<boost::try_mutex> lock(mutex_);
    while (running_)
        cond_.wait(lock);
}

void Server::startAccept()
{
#ifndef FATIGUE_MANAGER_NO_ENCRYPTION

    SocketPtr socket = SocketPtr(
        new SocketType(COMMS_ADDR_FM, COMMS_ADDR_RD, service_, context_));
    acceptor_.async_accept(socket->next_layer().next_layer(), remoteAddress_,
        boost::bind(&Server::handleAccept, this, socket, _1));

#else // !FATIGUE_MANAGER_NO_ENCRYPTION

    SocketPtr socket = SocketPtr(
        new SocketType(COMMS_ADDR_FM, COMMS_ADDR_RD, service_));
    acceptor_.async_accept(socket->next_layer(), remoteAddress_,
        boost::bind(&Server::handleAccept, this, socket, _1));

#endif // !FATIGUE_MANAGER_NO_ENCRYPTION

}

void Server::handleAccept(
    SocketPtr socket,
    const boost::system::error_code &ec)
{
    std::string remoteAddress =
	    socket->lowest_layer().remote_endpoint().address().to_string();
    
    if (ec == boost::asio::error::operation_aborted)
    {
        Log::warning("Accept Aborted: %s - %s", ec.message(), remoteAddress);
        return;
    }
    else if (ec)
    {
        Log::warning("Accept Error: %s - %s", ec.message(), remoteAddress);
        startAccept();
        return;
    }
    
    Log::debug("Accepting Connection: %s", remoteAddress);

    // Make sure we restart our accept in case of error
    Util::scopeguard acceptGuard = Util::make_obj_guard(*this, &Server::startAccept);
    static_cast<void>(acceptGuard);

#ifndef FATIGUE_MANAGER_NO_ENCRYPTION

    // Start our dtls handshake
    socket->next_layer().async_handshake(SocketType::next_layer_type::server,
        boost::bind(&Server::handleDtlsHandshake, this, socket, _1));

#else // !FATIGUE_MANAGER_NO_ENCRYPTION

    // Start our actual connection handshake
    boost::shared_array<std::uint8_t> readBuffer(new std::uint8_t[OldPacketSize]);
    socket->set_async_filter(boost::asio::buffer(OldPacketFilter),
        boost::asio::buffer(OldPacketFilterMask),
        boost::asio::buffer(readBuffer.get(), OldPacketSize),
        boost::bind(&Server::handleOldPacket, this, socket, readBuffer, _1, _2));
    socket->async_handshake(Asio::Comms::Handshake::Server, boost::bind(
        &Server::handleHandshake, this, socket, _1));

#endif // !FATIGUE_MANAGER_NO_ENCRYPTION

}

#ifndef FATIGUE_MANAGER_NO_ENCRYPTION

void Server::handleDtlsHandshake(
    SocketPtr socket,
    const boost::system::error_code &ec)
{
    if (ec == boost::asio::error::operation_aborted)
    {
        Log::warning("DtlsHandshake Aborted: %s", ec.message());
        return;
    }
    else if (ec)
    {
        Log::warning("DtlsHandshake Error: %s", ec.message());
        return;
    }

    // Start our actual connection handshake
    boost::shared_array<std::uint8_t> readBuffer(new std::uint8_t[OldPacketSize]);
    socket->set_async_filter(boost::asio::buffer(OldPacketFilter),
        boost::asio::buffer(OldPacketFilterMask),
        boost::asio::buffer(readBuffer.get(), OldPacketSize),
        boost::bind(&Server::handleOldPacket, this, socket, readBuffer, _1, _2));
    socket->async_handshake(Asio::Comms::Handshake::Server, boost::bind(
        &Server::handleHandshake, this, socket, _1));
}

#endif // FATIGUE_MANAGER_NO_ENCRYPTION

void Server::handleHandshake(
    SocketPtr socket,
    const boost::system::error_code &ec)
{
    socket->set_async_filter(boost::asio::const_buffer(),
        boost::asio::const_buffer(), boost::asio::mutable_buffer(),
        SocketType::ReadHandler());

    if (ec)
    {
        if (ec != Asio::Comms::make_error_code(comms_not_connected))
            Log::warning("Error connecting to display: %s %s",
		    ec.message(),
		    socket->lowest_layer().remote_endpoint().address().to_string());
        return;
    }

    // Socket is connected, create our session
    sessionManager_->create(socket);
}

void Server::handleOldPacket(
    SocketPtr socket,
    boost::shared_array<std::uint8_t> bufferPtr,
    const boost::system::error_code &ec,
    std::size_t size)
{
    if (ec || !size)
        return;

    sessionManager1_->create(std::move(socket->move_next_layer()),
        bufferPtr.get(), size);
}


void Server::handleAlarmRuleUpdate(const boost::system::error_code &ec)
{
    if (ec == boost::asio::error::operation_aborted)
        return;
    else if (ec)
    {
        Log::warning("Timer Error (ARUT): %s", ec.message());
        return;
    }

    // Reset our alarm
    alarmRuleUpdateTimer_.expires_from_now(
        boost::posix_time::minutes(AlarmRuleUpdateIntervalMinutes));
    alarmRuleUpdateTimer_.async_wait(
        boost::bind(&Server::handleAlarmRuleUpdate, this, _1));

    sessionManager_->checkAlarmRulesChanged();
    sessionManager_->checkPlantGroupsChanged();
    sessionManager1_->checkAlarmRulesChanged();
}

void Server::handleFirmwareUpdate(const boost::system::error_code &ec)
{
    if (ec == boost::asio::error::operation_aborted)
        return;
    else if (ec)
    {
        Log::warning("Timer Error (FUT): %s", ec.message());
        return;
    }

    // Reset our alarm
    firmwareUpdateTimer_.expires_from_now(
        boost::posix_time::minutes(FirmwareUpdateIntervalMinutes));
    firmwareUpdateTimer_.async_wait(
        boost::bind(&Server::handleFirmwareUpdate, this, _1));

    sessionManager_->checkPlantRequireFWCheck();
}

void Server::handleServerTimeout()
{
    namespace Email = ServiceUtil::Email;

    ServiceUtil::Config::const_iterator iter;
    bool smtpServerConfigured = false;
    if ((iter = config_.find(SmtpServerConfig)) != config_.end() &&
        !iter->second.empty())
        smtpServerConfigured = true;

    std::string recipients = defaultAt(config_, ServerCommsRecipientConfig,
        std::string());

    if (smtpServerConfigured && !recipients.empty())
    {
        // Before we send email off, check if at least one of our plants is online
        Email::send(config_.at(SmtpServerConfig),
            recipients,
            config_.at(MailSenderConfig),
            config_.at(ServerCommsSubjectConfig),
            config_.at(ServerCommsBodyConfig));
    }
}

void Server::handlePlantOfflineTimeout(const Sql::IdType &plantDeviceId)
{
    using boost::replace_all_copy;
    namespace Email = ServiceUtil::Email;

    // Check if we have a smtp server configured
    ServiceUtil::Config::const_iterator iter;
    bool smtpServerConfigured = false;
    if ((iter = config_.find(SmtpServerConfig)) != config_.end() &&
        !iter->second.empty())
        smtpServerConfigured = true;

    // Loop through and process our offline plants
    std::string recipients = defaultAt(config_, PlantOfflineRecipientConfig,
        std::string());

    if (smtpServerConfigured && !recipients.empty())
    {
        std::string plantAlias;
        {
            DatabasePool::ResourcePtr sql = resources_.databasePool().acquire();
            plantAlias = Sql::plantName(*sql, plantDeviceId);
        }

        Email::send(config_.at(SmtpServerConfig),
            recipients,
            config_.at(MailSenderConfig),
            replace_all_copy(config_.at(PlantOfflineSubjectConfig),
                MailSubst, plantAlias),
            replace_all_copy(config_.at(PlantOfflineBodyConfig),
                MailSubst, plantAlias));
    }

}

void Server::handleAlarmNotification(const Sql::IdType &plantDeviceId)
{
    if (!notificationSocket_)
    {
        try
        {
            std::pair<boost::asio::io_service &, std::string> pair(service_,
                notificationAddress_);
            notificationSocket_.reset(socketRegistry_.create(
                notificationType_, pair));
            notificationInProgress_ = false;
        }
        catch(...)
        {}
    }

    if (!notificationSocket_)
        return;

    notificationAlarms_.push_back(plantDeviceId);

    beginNotification();
}

void Server::beginNotification()
{
    if (notificationInProgress_ || notificationAlarms_.empty() || !running_)
        return;

    auto id = notificationAlarms_.front();
    notificationAlarms_.pop_front();

    notificationBuffer_.clear();
    auto iter = std::back_inserter(notificationBuffer_);
    Util::packBig<2>(iter, AlarmNotificationType);
    Util::packBig<8>(iter, id);

    notificationSocket_->async_send(boost::asio::buffer(notificationBuffer_),
        std::bind(&Server::handleNotificationSend, this, std::placeholders::_1));
}

void Server::handleNotificationSend(const boost::system::error_code &ec)
{
    notificationInProgress_ = false;

    if (ec)
    {
        notificationSocket_.reset();
        notificationAlarms_.clear();
        return;
    }

    beginNotification();
}

void Server::cancelAsync()
{
    service_.reset();
    alarmRuleUpdateTimer_.cancel();
    firmwareUpdateTimer_.cancel();
    acceptor_.cancel();
    service_.poll();
    service_.stop();
}

} // FatigueManager namespace
